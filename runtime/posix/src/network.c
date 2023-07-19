#include "nbp/network.h"
#include "nb_runtime.h"
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <assert.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#define MAX_FDS (256)

#define DEBUG (1)

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
	usleep(100 * 1000);	
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

// The arguments to this function are completely ignored
// the stack will support only one type of socket. 

// If the generated stack has parameters/variations, 
// we could use these in the future
int nbp_socket(int domain, int type, int protocol) {
#if DEBUG
	printf("nbp_socket called\n");
#endif
	int fd = grab_free_fd();
	// this fd has just been reserved, we will create it when we 
	// have all the parameters	
	fd_table[fd] = NULL;
	fd_accept_counter[fd] = 0;

	return fd;
}


// A common callback function that handles all event changes
static void nb_posix_common_callback(int event, nb__connection_t * c) {
	if (event == QUEUE_EVENT_ACCEPT_READY) {
		int fd = fd_from_connection(c);
#ifdef DEBUG
		printf("Accept event fired on fd = %d\n", fd);
#endif 
		fd_accept_counter[fd]++;
	}
	return;
}

// These are the assignable source ports 
// for connect
static unsigned int port_counter = 20;

int nbp_connect (int sockfd, const struct sockaddr* _addr, socklen_t addrLen) {
#if DEBUG
	printf("nbp_connect called with fd = %d\n", sockfd);
#endif
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return connect(sockfd, _addr, addrLen);
	}
	// Assuming the address are TCP like
	assert(addrLen >= sizeof(struct sockaddr_in));
	const struct sockaddr_in* addr = (void*) _addr;
	
	// a connect needs the destination address and port
	// and we need to assign a port for source	

	unsigned int src_port = port_counter++;
	unsigned int dst_port = addr->sin_port;
	unsigned long dst_addr = addr->sin_addr.s_addr;
	unsigned char dst_addr_char[6] = {0, 0, 0, 0, 0, 0};
	memcpy(dst_addr_char, &dst_addr, sizeof(dst_addr_char));
	
	// we can now actually establish the connection and create a connection object
	assert(nb_fd[sockfd] == 1);
	assert(fd_table[sockfd] == NULL);
	
	fd_table[sockfd] = nb__establish(dst_addr_char, dst_port, src_port, nb_posix_common_callback);
	nb__set_user_data(fd_table[sockfd], (void*)(size_t) sockfd);

	// This currently isn't required to be called repeatedly, but when establish is changed to 
	// fire an event, we will run this till establish event is called
	nb__main_loop_step();	
	return 0;
}

int nbp_bind(int sockfd, const struct sockaddr *_addr, socklen_t addrlen) {
#if DEBUG
	printf("nbp_bind called with fd = %d\n", sockfd);
#endif
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return bind(sockfd, _addr, addrlen);
	}
	// Assuming the address are TCP like
	assert(addrlen >= sizeof(struct sockaddr_in));
	const struct sockaddr_in* addr = (void*) _addr;

	unsigned int src_port = addr->sin_port;
	// 0 is the wildcard for destination ports
	unsigned int dst_port = 0;
	// Set the dst addr to also be wildcard
	char* dst_addr = nb__wildcard_host_identifier;

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
#if DEBUG
	printf("nbp_listen called with fd = %d\n", sockfd);
#endif
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return listen(sockfd, backlog);
	}
	// Our implementation has to implementation of listen
	// so this is a noop
	
	// Just make sure this is our fd
	assert(nb_fd[sockfd] == 1);
	return 0;
}



int nbp_accept(int sockfd, struct sockaddr *addr, socklen_t * addrlen) {
#if DEBUG
	printf("NBP_ACCEPT INVOKED ON FD = %d\n", sockfd);
#endif
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return accept(sockfd, addr, addrlen);
	}
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
	
	return fd;
}

ssize_t nbp_send(int sockfd, const char buf[], size_t len, int flags) {
#if DEBUG
	printf("nbp_send called with fd = %d\n", sockfd);
#endif
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return send(sockfd, buf, len, flags);
	}
	// Just make sure this is our fd
	assert(nb_fd[sockfd] == 1);
	// the flags will be ignored for this implementation
	return nb__send(fd_table[sockfd], (char*)buf, len);
}

ssize_t nbp_recv(int sockfd, char buf[], size_t len, int flags) {
#if DEBUG
	printf("nbp_recv called with fd = %d\n", sockfd);
#endif
	if (nb_fd[sockfd] != 1) {
		// this fd does not belong to us
		return recv(sockfd, buf, len, flags);
	}
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

int nbp_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
#if DEBUG
	printf("SETSOCKOPT called on fd=%d\n", sockfd);
#endif
	if (nb_fd[sockfd] != 1) {
		return setsockopt(sockfd, level, optname, optval, optlen);
	}
	// We will leave setsockopt unimplemented for now
	// if applications require us to return certain meaningful values, we could support
	// those
	return 0;
}
int nbp_ioctl(int fd, unsigned long request, char* argp) {
#if DEBUG
	printf("nbp_ioctl called with fd = %d\n", fd);
#endif
	if (nb_fd[fd] != 1) {
		return ioctl(fd, request, argp);
	}
	// We will leave ioctl unimplemented for now
	// if applications require us to return certain meaningful values, we could support 
	// those	
	return 0;
}

int nbp_close(int fd) {
#if DEBUG
	printf("nbp_close called with fd = %d\n", fd);
#endif
	if (nb_fd[fd] != 1) {
		return close(fd);
	}
	// TODO: implement close
	return 0;
}

int nbp_init_server(const char* ip) {
	nb__ipc_init("/tmp/ipc_socket", 1);
	nb__net_init();
	unsigned long my_ip = inet_addr(ip);
	memcpy(nb__my_host_id, &my_ip, 6);
	printf("nbp_init_server complete\n");
	return 0;
}
int nbp_init_client(const char* ip) {
	nb__ipc_init("/tmp/ipc_socket", 0);
	nb__net_init();
	unsigned long my_ip = inet_addr(ip);
	memcpy(nb__my_host_id, &my_ip, 6);
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
	// for the nbp implementation we will ingore the writefd and the exceptfds, they are always ready
	// We will also assume that all fds are belong to us	
	int fds[MAX_FDS];
	int fd_count = 0;
	// We will first prepare a sparse set of fds
	for (int fd = 0; fd < nfds; fd++) {
		if (FD_ISSET(fd, readfds)) {
			fds[fd_count++] = fd;
		}
	}
#if DEBUG
	printf("Called select with fds = ");
	for (int i = 0; i < fd_count; i++) {
		int fd = fds[i];
		printf("%d, ", fd);
	}
	printf("\n");
#endif
	FD_ZERO(readfds);
	// We have a set of fds ready to go now
	// check if any of them have accept or read events ready	
	while (1) {
		nb__main_loop_step();	
		for (int i = 0; i < fd_count; i++) {
			int fd = fds[i];
			if (fd_accept_counter[fd] != 0) {
#if DEBUG
			printf("Read ready fd = %d\n", fd);
#endif
				FD_SET(fd, readfds);
				return 1;
			}
		}
		nb_step_delay();
	}
}
