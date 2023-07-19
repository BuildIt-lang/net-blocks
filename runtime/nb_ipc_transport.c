#include "nb_runtime.h"
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
static int ipc_socket;

#define IPC_MTU (1024)
int nb__ipc_simulate_out_of_order = 0;
int nb__ipc_simulate_packet_drop = 0;
static char out_of_order_store[IPC_MTU];
static int out_of_order_len = 0;
#define OUT_OF_ORDER_CHANCE (5)
#define PACKET_DROP_CHANCE (5)

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
		fcntl(ipc_socket, F_SETFL, O_NONBLOCK);
	} else {
		// Delete any stale sockets
		unlink(sock_path);
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
		fcntl(ipc_socket, F_SETFL, O_NONBLOCK);
		close(ipc_fd);
	}
}
void nb__ipc_deinit(void) {
	// Send any ooo packet we are hoarding
	if (out_of_order_len) {
		int ret = write(ipc_socket, out_of_order_store, out_of_order_len);
		out_of_order_len = 0;
	}
	close(ipc_socket);
}
char nb__reuse_mtu_buffer[IPC_MTU];
char* nb__poll_packet(int* size, int headroom) {
	int len;
	static char temp_buf[IPC_MTU];
	len = (int) read(ipc_socket, temp_buf, IPC_MTU);
	*size = 0;
	if (len > 0) {		
		char* buf = malloc(IPC_MTU + headroom);
		memcpy(buf + headroom, temp_buf, IPC_MTU);
		*size = len;
		//nb__debug_packet(temp_buf);
		
		return buf;
	}
	return NULL;
}
int nb__send_packet(char* buff, int len) {
	if (nb__ipc_simulate_packet_drop) {
		int r = rand() % PACKET_DROP_CHANCE;
		if (r == 0) {
			//nb__debug_packet(buff);
			return len;
		}
	}
	// Don't try to out of order if we already have on pending
	if (out_of_order_len == 0 && nb__ipc_simulate_out_of_order) {
		int r = rand() % OUT_OF_ORDER_CHANCE;
		if (r == 0) {
			out_of_order_len = len;
			memcpy(out_of_order_store, buff, len);
			return len;
		}
	}
	//nb__debug_packet(buff);
	int ret = write(ipc_socket, buff, len);
	// If there is a pending packet, send it now
	if (out_of_order_len) {
		int ret = write(ipc_socket, out_of_order_store, out_of_order_len);
		out_of_order_len = 0;
	}
	return ret;
}


char* nb__request_send_buffer(void) {
	return malloc(IPC_MTU);
}
void* nb__return_send_buffer(char* p) {
	free(p);
}
