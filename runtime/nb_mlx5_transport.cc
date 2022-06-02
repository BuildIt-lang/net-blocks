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
#include "mlx5_defs.h"


static Transport * transport;
#define MLX5_MTU (1024)


int nb__mlx5_simulate_out_of_order = 0;
int nb__mlx5_simulate_packet_drop = 0;
msgbuffer out_of_order_buffer;
int out_of_order_stored = 0;
#define OUT_OF_ORDER_CHANCE (5)
#define PACKET_DROP_CHANCE (5)

char nb__reuse_mtu_buffer[MLX5_MTU];

void nb__mlx5_init(void) {
	transport = new Transport();	
}

static msgbuffer* get_next_send_buffer(void) {
	int idx = __sync_fetch_and_add(&(transport->send_buffer_index), 1);
	idx = idx % 512;
	return &(transport->send_buffers[idx]);
}
char* nb__request_send_buffer(void) {
	msgbuffer* buffer = get_next_send_buffer();
	return buffer->buffer;
}
void* nb__return_send_buffer(char*) {
	// This implementation is empty
	// We have enough buffers and we assume that a 
	// buffer is returned before it is used again
}

int nb__send_packet(char* buff, int len) {

	if (nb__mlx5_simulate_packet_drop) {
		int r = rand() % PACKET_DROP_CHANCE;
		if (r == 0) {
			return len;
		}
	}

	int buffer_index = (buff - transport->main_send_buffer) / (MLX5_MTU);
	msgbuffer *buffer = &(transport->send_buffers[buffer_index]);
	// Retrieve key from original buffer
	msgbuffer b;
	b.buffer = buff;	
	b.length = len;
	b.lkey = buffer->lkey;

	//nb__debug_packet(buff);
	transport->send_message(&b);	
}

static int pending_recv = 0;
char* nb__poll_packet(int* size, int headroom) {
	*size = 0;
	unsigned char *packet_addr;
	if (pending_recv != 0) {	
		packet_addr = transport->recv_buffer + (transport->recv_idx % 4096) * IBV_FRAME_SIZE;
		transport->recv_idx++;
		*size = MLX5_MTU;
		return (char*)packet_addr;
	}
		
	int delta = transport->try_recv();
	if (delta == 0) {	
		return nullptr;
	}
	packet_addr = transport->recv_buffer + (transport->recv_idx % 4096) * IBV_FRAME_SIZE;
	transport->recv_idx++;
	pending_recv = delta - 1;
	*size = MLX5_MTU;
	transport->recvs_to_post += delta;

	// If we have used 512 recv buffers, post them to be used again	
	if (transport->recvs_to_post > 512) {
		int idx = transport->recv_sge_idx;
		transport->recvs_to_post -= 512;
		transport->recv_sge_idx = (idx + 1) % 8;
		struct ibv_sge* sge = transport->mp_recv_sge + idx;
		//wq_recv_burst(transport->wq, sge, 1);
		transport->wq_family->recv_burst(transport->wq, sge, 1);
	}
	*size = MLX5_MTU;
	// Make space for non-transport headers
	// Currently we are stealing from the previous packet
	packet_addr = packet_addr - headroom;
	return (char*)packet_addr;				
}
