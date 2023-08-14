#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define SOCK_PATH ("/tmp/ipc_socket")
char buffer[1600];

int main(int argc, char* argv[]) {

	int ipc_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (ipc_socket < 0) {
		fprintf(stderr, "Socket create failed\n");
		exit(-1);	
	}	

	// delete any existing sockets
	unlink(SOCK_PATH);


	// Bind mode -- only the hub is ever in bind mode
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, SOCK_PATH);

	if (bind(ipc_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "Socket bind failed\n");
		exit(-1);
	}
	listen(ipc_socket, 10);

	// main loop

	int fds[256];	
	int fd_count = 0;

	while (1) {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(ipc_socket, &set);
		int fd_max = ipc_socket;
		for (int i = 0; i < fd_count; i++) {
			FD_SET(fds[i], &set);
			if (fds[i] > fd_max) fd_max = fds[i];	
		}
		int ret = select(fd_max + 1, &set, NULL, NULL, NULL);
		if (ret < 0) {
			fprintf(stderr, "Select returned -1\n");
			exit(-1);
		}
		// First accept on the main socket
		if (FD_ISSET(ipc_socket, &set)) {
			int new_fd = accept(ipc_socket, NULL, NULL);
			if (new_fd < 0) {
				fprintf(stderr, "Accept returned -1\n");
				exit(-1);
			}
			fds[fd_count++] = new_fd;
			printf("New connection = %d\n", new_fd);
		}
		for (int i = 0; i < fd_count; i++) {
			if (FD_ISSET(fds[i], &set)) {
				int len = read(fds[i], buffer, sizeof(buffer));
				if (len == 0) {
					// this socket has closed
					printf("Closing connection = %d\n", fds[i]);
					close (fds[i]);	
					// swap with the last
					fds[i] = fds[fd_count-1];
					fd_count--;	
					// also decrement i so the swapped out one is processed 
					i--;
				} else {
					// There is some data, relay it to others
					//printf("Received %d bytes from %d\n", len, fds[i]);	
					for (int j = 0; j < fd_count; j++) {
						if (i == j) continue;	
						// It is okay if this fails -- some sockets might have closed
						send(fds[j], buffer, len, 0);
						//printf("Sent %d bytes to %d\n", len, fds[j]);
					}
				}
			}
		}
	}
	

	return 0;
}
