#include "nb_runtime.h"
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>

#include <infiniband/verbs.h>
#include <iostream>
#include "halloc.h"
#include <arpa/inet.h>
#include "transport.h"


static struct transport_t transport;
#define MLX5_MTU (1024)


char nb__reuse_mtu_buffer[MLX5_MTU];

void nb__mlx5_init(const char* name) {
	mlx5_try_transport(&transport, name);
}

char* nb__request_send_buffer(void) {
	return mlx5_get_next_send_buffer(&transport);
}
void* nb__return_send_buffer(char*) {
	// This implementation is empty
	// We have enough buffers and we assume that a 
	// buffer is returned before it is used again
	return nullptr;
}

int nb__send_packet(char* buff, int len) {
	//printf("Sending:\n");
        //nb__debug_packet(buff, 60);
	mlx5_try_send(&transport, buff, len);	

	return len;
}

static int pending_recv = 0;

char* nb__poll_packet(int* size, int headroom) {

	*size = 0;
	if (pending_recv == 0) 
		pending_recv = mlx5_try_recv(&transport);

	if (pending_recv != 0) {
		char* packet_addr = mlx5_get_next_recv_buffer(&transport);
		*size = MLX5_MTU;
		packet_addr = packet_addr - headroom;
		//printf("Receiving:\n");
        	//nb__debug_packet(packet_addr, 60);

		pending_recv--;
		return packet_addr;
	}
	return nullptr;
	
}

void nb__transport_default_init(void) {
	nb__mlx5_init("eth2");
}
void nb__transport_default_deinit(void) {
}
