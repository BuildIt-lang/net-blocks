#include "nb_runtime.h"
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
static int ipc_socket;

void nb__ipc_init(const char* sock_path, int mode) {
	ipc_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (ipc_socket < 0) {
		fprintf(stderr, "Socket create failed\n");
		exit(-1);
	}

	if (mode == 0) {
		// connect mode
		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, sock_path);
		if (connect(ipc_socket, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) < 0) {
			fprintf(stderr, "Socket connect failed\n");
			exit(-1);
		}
	} else {
		// Bind mode
		struct sockaddr_un addr;
		int ipc_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
		if (ipc_fd < 0) {
			fprintf(stderr, "Socket create failed\n");
			exit(-1);
		}
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, sock_path);
		if (bind(ipc_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			fprintf(stderr, "Socket bind failed\n");
			exit(-1);
		}
		listen(ipc_fd, 1);
		ipc_socket = accept(ipc_fd, NULL, NULL);
		if (ipc_socket < 0) {
			fprintf(stderr, "Socket accept failed\n");
			exit(-1);
		}
		close(ipc_fd);
	}
}
#define IPC_MTU (1024)
char* nbr__poll_packet(int* size) {
	int len;
	static char temp_buf[IPC_MTU];
	len = (int) read(ipc_socket, temp_buf, IPC_MTU);
	*size = 0;
	if (len > 0) {
		char* buf = malloc(IPC_MTU);
		memcpy(buf, temp_buf, IPC_MTU);
		*size = len;
		return buf;
	}
	return NULL;
}
int nbr__send_packet(char* buff, int len) {
	return (int) write(ipc_socket, buff, len);	
}
