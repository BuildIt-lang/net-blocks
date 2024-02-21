#include "nbp/network.h"
#include "nb_runtime.h"
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <assert.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#define MAX_FDS (256)


// static table to keep track of connection objects
static nb__connection_t* fd_table[MAX_FDS];
// static table to keep track if a particular fd is owned by netblocks
static int nb_fd[MAX_FDS];

static int fd_from_connection(nb__connection_t* conn) {
	// For now we will have to scan the fd table to find the fd
	// TODO: change nb to allow user data in the connection object
	int fd = (int) (size_t) nb__get_user_data(conn);
	return fd;
}

static void nb_step_delay(void) {
	// This is a configurable delay function, 
	// in real time-low latency applications that doesn't require OS intervention
	// this will be NOP
	//usleep(100);	
}

static int grab_free_fd(void) {
	// unfortunately getting a new fd is hard, so we ask the OS
	// this will be closed only when the nb_socket is closed
	int fd = open("/dev/zero", 0, O_RDONLY);
	assert(nb_fd[fd] == 0);
	nb_fd[fd] = 1;
	return fd;	
}

volatile static unsigned fd_accept_counter[MAX_FDS];
static unsigned fd_establish_status[MAX_FDS];

static int port_occupied[100000 - 8000]; // Flags to check if port is free
static unsigned short fd_src_port[MAX_FDS];

// The arguments to this function are completely ignored
// the stack will support only one type of socket. 

// If the generated stack has parameters/variations, 
// we could use these in the future
int nbp_socket(int domain, int type, int protocol) {
	int fd = grab_free_fd();
	// this fd has just been reserved, we will create it when we 
	// have all the parameters	
	fd_table[fd] = NULL;
	fd_accept_counter[fd] = 0;
	fd_establish_status[fd] = 0;
	fd_src_port[fd] = 0;
	nbp_debug("nbp_socket returning = %d", fd);

	return fd;
}


// A common callback function that handles all event changes
static void nb_posix_common_callback(int event, nb__connection_t * c) {
	if (event == QUEUE_EVENT_ACCEPT_READY) {
		int fd = fd_from_connection(c);
		nbp_debug("Callback event on fd = %d", fd);
		fd_accept_counter[fd]++;
	} else if (event == QUEUE_EVENT_ESTABLISHED) {
		int fd = fd_from_connection(c);
		nbp_debug("Callback event on fd = %d", fd);
		fd_establish_status[fd] = 1;
	}
	return;
}

// These are the assignable source ports 
// for connect
static unsigned int find_free_port(void) {
	for (int i = 8080; i < 10000; i++) {
		if (!port_occupied[i - 8000]) {
			port_occupied[i - 8000] = 1;
			return i;
		}
	}
	assert(0);
	return 0;
}

int nbp_connect (int sockfd, const struct sockaddr* _addr, socklen_t addrLen) {

	nbp_debug("nbp_connect with fd = %d", sockfd);	

	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return connect(sockfd, _addr, addrLen);
	}
	// Assuming the address are TCP like
	assert(addrLen >= sizeof(struct sockaddr_in));
	const struct sockaddr_in* addr = (void*) _addr;
	
	// a connect needs the destination address and port
	// and we need to assign a port for source	

	unsigned int src_port = find_free_port();
	unsigned int dst_port = addr->sin_port;
	unsigned int dst_addr = addr->sin_addr.s_addr;
	
	// we can now actually establish the connection and create a connection object
	assert(nb_fd[sockfd] == 1);
	assert(fd_table[sockfd] == NULL);
	
	fd_table[sockfd] = nb__establish(dst_addr, dst_port, src_port, nb_posix_common_callback);
	nb__set_user_data(fd_table[sockfd], (void*)(size_t) sockfd);

	while (fd_establish_status[sockfd] != 1) {
		nb__main_loop_step();	
		nb_step_delay();
	}
	return 0;
}

int nbp_bind(int sockfd, const struct sockaddr *_addr, socklen_t addrlen) {
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return bind(sockfd, _addr, addrlen);
	}
	nbp_debug("nbp_bind with fd = %d", sockfd);	
	// Assuming the address are TCP like
	assert(addrlen >= sizeof(struct sockaddr_in));
	const struct sockaddr_in* addr = (void*) _addr;

	unsigned int src_port = addr->sin_port;
	// 0 is the wildcard for destination ports
	unsigned int dst_port = 0;
	// Set the dst addr to also be wildcard
	unsigned long long dst_addr = nb__wildcard_host_identifier;

	// we can now actually establish the connection and create a connection object
	assert(nb_fd[sockfd] == 1);
	assert(fd_table[sockfd] == NULL);
	
	fd_table[sockfd] = nb__establish(dst_addr, dst_port, src_port, nb_posix_common_callback);
	nb__set_user_data(fd_table[sockfd], (void*)(size_t)sockfd);
	// This currently isn't required to be called repeatedly, but when establish is changed to 
	// fire an event, we will run this till establish event is called
	nb__main_loop_step();	

	return 0;
}


int nbp_listen(int sockfd, int backlog) {
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return listen(sockfd, backlog);
	}
	nbp_debug("nbp_listen with fd = %d", sockfd);	
	// Our implementation has to implementation of listen
	// so this is a noop
	
	// Just make sure this is our fd
	assert(nb_fd[sockfd] == 1);
	return 0;
}



int nbp_accept(int sockfd, struct sockaddr *addr, socklen_t * addrlen) {
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		int fd = accept(sockfd, addr, addrlen);
		return fd;
	}
	nbp_debug("nbp_accept with fd = %d", sockfd);	
	// Just make sure this is our fd
	assert(nb_fd[sockfd] == 1);
	// Accept requires the accept event to fire	
	// so we will wait for it to happen	
	while(fd_accept_counter[sockfd] == 0) {
		nb__main_loop_step();	
		nb_step_delay();
	}
	// A new connection is ready to be accepted
	int fd = grab_free_fd();
	fd_table[fd] = nb__accept(fd_table[sockfd], nb_posix_common_callback);
	fd_accept_counter[sockfd]--;
	nb__set_user_data(fd_table[fd], (void*)(size_t)fd);

	// This currently isn't required to be called repeatedly, but if accept-done is changed to 
	// fire an event, we will run this till accept-done event is called
	nb__main_loop_step();	

	if (addr != NULL && addrlen != NULL) {
		// Return the accept information otherwise apps might complain
		struct sockaddr_in* addr_in = (void*) addr;
		addr_in->sin_family = AF_INET;
		addr_in->sin_port = fd_table[fd]->remote_app_id;
		addr_in->sin_addr.s_addr = 0;
		//memcpy(&(addr_in->sin_addr.s_addr), fd_table[fd]->remote_host_id, sizeof(addr_in->sin_addr.s_addr));
		addr_in->sin_addr.s_addr = fd_table[fd]->remote_host_id;
		*addrlen = sizeof(struct sockaddr_in);
	}
	
	return fd;
}

ssize_t nbp_send(int sockfd, const char buf[], size_t len, int flags) {
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return send(sockfd, buf, len, flags);
	}
	nbp_debug("nbp_send with fd = %d", sockfd);	
	// Just make sure this is our fd
	assert(nb_fd[sockfd] == 1);
	// the flags will be ignored for this implementation
	return nb__send(fd_table[sockfd], (char*)buf, len);
}

ssize_t nbp_write(int sockfd, const char buf[], size_t len) {


	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return write(sockfd, buf, len);
	}

	nbp_debug("nbp_write with fd = %d", sockfd);	

	// Just make sure this is our fd
	assert(nb_fd[sockfd] == 1);
	// the flags will be ignored for this implementation
	return nb__send(fd_table[sockfd], (char*)buf, len);
	
}
ssize_t nbp_writev(int sockfd, const struct iovec *iov, int iovcnt) {
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return writev(sockfd, iov, iovcnt);
	}
	nbp_debug("nbp_writev with fd = %d", sockfd);	
	size_t len_sent = 0;
	for (int i = 0; i < iovcnt; i++) {
		size_t l = nb__send(fd_table[sockfd], iov[i].iov_base, iov[i].iov_len);
		len_sent += l;
		if (l != iov[i].iov_len)
			break;
	}
	return len_sent;
}

ssize_t nbp_recv(int sockfd, char buf[], size_t len, int flags) {
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return recv(sockfd, buf, len, flags);
	}
	nbp_debug("nbp_recv with fd = %d", sockfd);	
	// Just make sure this is our fd
	assert(nb_fd[sockfd] == 1);
	// Ideally we would want to wait for read ready events, but nb__read has a non-blocking API
	// so we can just spin till we get something

	// flags are ignored for now
	nb__connection_t* conn = fd_table[sockfd];
	ssize_t ret_len;
	while ((ret_len = nb__read(conn, buf, len)) == 0) {
		nb__main_loop_step();	
		nb_step_delay();
	}
	return ret_len;	
}
ssize_t nbp_read(int sockfd, char buf[], size_t len) {
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return read(sockfd, buf, len);
	}
	nbp_debug("nbp_read with fd = %d", sockfd);	
	return nbp_recv(sockfd, buf, len, 0);	
}

int nbp_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
	if (nb_fd[sockfd] != 1) {
		return setsockopt(sockfd, level, optname, optval, optlen);
	}
	nbp_debug("nbp_setsockopt with fd = %d", sockfd);	
	// We will leave setsockopt unimplemented for now
	// if applications require us to return certain meaningful values, we could support
	// those
	return 0;
}
int nbp_ioctl(int fd, unsigned long request, char* argp) {
	if (nb_fd[fd] != 1) {
		return ioctl(fd, request, argp);
	}
	nbp_debug("nbp_ioctl with fd = %d", fd);	
	// We will leave ioctl unimplemented for now
	// if applications require us to return certain meaningful values, we could support 
	// those	
	return 0;
}

int nbp_close(int fd) {
	if (nb_fd[fd] != 1) {
		return close(fd);
	}
	nbp_debug("nbp_close with fd = %d", fd);	
	nb__connection_t* conn = fd_table[fd];
	nb__destablish(conn);	
	// We will immediately deregister this fd and 
	// close the underlying file	
	// If we add a close event, we will wait for 
	// the event
	// Free up the port to be used again
	port_occupied[fd_src_port[fd]] = 0;
	fd_table[fd] = NULL;
	nb_fd[fd] = 0;
	close(fd);
	return 0;
}

int nbp_default_init(const char* ip) {
	nb__transport_default_init();
	unsigned int my_ip = inet_addr(ip);
	nb__my_host_id = my_ip;
	nb__net_init();
	return 0;	
}

int nbp_pause(void) {
	while (1) {
		nb__main_loop_step();	
		nb_step_delay();
	}
	return 0;
}

int nbp_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {

	// nbp_debug("nbp_select with nfds = %d", nfds);	

	// for the nbp implementation we will ingore the writefd and the exceptfds, they are always ready
	// We will also assume that all fds are belong to us	
	int fds[MAX_FDS];
	int fd_count = 0;
	// We will first prepare a sparse set of fds
	for (int fd = 0; fd < nfds; fd++) {
		if (FD_ISSET(fd, readfds)) {
			fds[fd_count++] = fd;
			// nbp_debug("> in set = %d\n", fd);
		}
	}

	FD_ZERO(readfds);
	// We have a set of fds ready to go now
	// check if any of them have accept or read events ready	
	size_t count = 0;
	while (count < 1000) {
		nb__main_loop_step();	
		for (int i = 0; i < fd_count; i++) {
			int fd = fds[i];
			if (fd_accept_counter[fd] != 0) {
				FD_SET(fd, readfds);
				nbp_debug("From nbp_select ready for accept fd = %d", fd);
				return 1;
			} 
			if (fd_table[fd]->input_queue->current_elems) {
				FD_SET(fd, readfds);
				nbp_debug("From nbp_select ready for read fd = %d", fd);
				return 1;
			}
		}
		nb_step_delay();
		count++;
	}
	return 0;
}
