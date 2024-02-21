#ifndef NETBLOCKS_POSIX_IMPL_H
#define NETBLOCKS_POSIX_IMPL_H
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>
// This header implements all the network functionality required by 
// posix style applications
// this calls into the NetBlocks generated code

// All the functions here have the nbp_ prefix
// We will use symbol renaming so that these don't conflict with existing 
// implementations


int nbp_default_init(const char* ip);

int nbp_socket (int domain, int type, int protocol);

int nbp_connect (int sockfd, const struct sockaddr* addr, socklen_t addrLen);

int nbp_accept(int sockfd, struct sockaddr *addr, socklen_t * addrlen);

int nbp_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int nbp_listen(int sockfd, int backlog);

ssize_t nbp_recv(int sockfd, char buf[], size_t len, int flags);

ssize_t nbp_send(int sockfd, const char buf[], size_t len, int flags);

int nbp_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

int nbp_ioctl(int fd, unsigned long request, char* arpg);

int nbp_close(int fd);

int nbp_pause(void);

int nbp_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

ssize_t nbp_write(int fd, const char buf[], size_t count);
ssize_t nbp_read(int fd, char buf[], size_t count);

ssize_t nbp_writev(int fd, const struct iovec *iov, int iovcnt);


// debugging stuff
#define NBP_DEBUG (0)

static inline void nbp_debug(const char* fmt, ...) {
#if NBP_DEBUG
	va_list args;
	va_start (args, fmt);
	fprintf(stdout, "NBP_DEBUG: ");
	vfprintf(stdout, fmt, args);
	fprintf(stdout, "\n");
	va_end(args);
#endif
}


#endif
