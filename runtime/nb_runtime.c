#include "nb_runtime.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

struct data_queue_t* nb__new_data_queue(void) {
	struct data_queue_t * q = malloc(sizeof(struct data_queue_t));
	q->current_elems = 0;
	return q;
}
void nb__free_data_queue(struct data_queue_t* q) {
	free(q);
}
void nb__insert_data_queue(struct data_queue_t* q, char* buff, int len) {
	int index = q->current_elems;
	q->current_elems++;
	q->data_queue_elems[index] = buff;
	q->data_queue_elems_size[index] = len;	
}

nb__accept_queue_t* nb__new_accept_queue(void) {
	nb__accept_queue_t* q = malloc(sizeof(*q));
	q->current_elems = 0;
	return q;
}
void nb__free_accept_queue(nb__accept_queue_t * q) {
	free(q);
}
void nb__insert_accept_queue(nb__accept_queue_t* q, unsigned src_app_id, unsigned long long src_host_id, void* packet) {

	int index = q->current_elems;
	q->current_elems++;
	q->src_app_id[index] = src_app_id;
	//memcpy(q->src_host_id[index], src_host_id, HOST_IDENTIFIER_LEN);
	q->src_host_id[index] = src_host_id;
	q->packet[index] = packet;
}

int nb__read(nb__connection_t* c, char* buff, int max_len) {
	if (c->input_queue->current_elems == 0)
		return 0;
	char* payload = c->input_queue->data_queue_elems[0];
	int payload_size = c->input_queue->data_queue_elems_size[0];
	int i;
	// TODO: Replace this with a circular queue
	for (i = 1; i < c->input_queue->current_elems; i++) {
		c->input_queue->data_queue_elems[i-1] = c->input_queue->data_queue_elems[i];
		c->input_queue->data_queue_elems_size[i-1] = c->input_queue->data_queue_elems_size[i];
	}
	c->input_queue->current_elems--;
	int copy_size = max_len < payload_size? max_len: payload_size;
	memcpy(buff, payload, copy_size);
	return copy_size;
}

nb__connection_t* nb__accept(nb__connection_t* c, void (*callback)(int, nb__connection_t*)) {
	if (c->accept_queue->current_elems == 0)
		return NULL;

	// We are popping from back, because order doesn't matter
	// TODO: This needs to be fixed!!
	// We should bump from the front otherwise we will get out of order packet 
	// and reordering of packets will cause drop
	int index = c->accept_queue->current_elems - 1;
	c->accept_queue->current_elems--;
	unsigned int src_app_id = c->accept_queue->src_app_id[index];
	unsigned long long src_host_id = c->accept_queue->src_host_id[index];
	unsigned int dst_app_id = c->local_app_id;
	void* packet = c->accept_queue->packet[index];
	nb__connection_t* s = nb__establish(src_host_id, src_app_id, dst_app_id, callback);
	
	// If is okay to pass 0 for now, because we know we aren't using the size	
	// TODO: Fix this later by storing size
	if (packet)
		nb__run_ingress_step(packet, 0);

	int i;
	for (i = 0; i < c->accept_queue->current_elems; i++) {
		if (c->accept_queue->src_app_id[i] == src_app_id 
			//&& memcmp(c->accept_queue->src_host_id[i], src_host_id, HOST_IDENTIFIER_LEN) == 0) {
			&& c->accept_queue->src_host_id[i] == src_host_id) {
			void* packet = c->accept_queue->packet[i];
			if (packet)
				nb__run_ingress_step(packet, 0);	
			// Swap this element with the last one
			int last = c->accept_queue->current_elems - 1;
			c->accept_queue->src_app_id[i] = c->accept_queue->src_app_id[last];
			//memcpy(c->accept_queue->src_host_id[i], c->accept_queue->src_host_id[last], HOST_IDENTIFIER_LEN);
			c->accept_queue->src_host_id[i] = c->accept_queue->src_host_id[last];
			c->accept_queue->packet[i] = c->accept_queue->packet[last];
			// decrement i so we don't miss a packet
			i--;
			// Bump the last elemnt 
			c->accept_queue->current_elems--;
		}
	}
	
	return s;
}
#define REDELIVERY_BUFFER_SIZE (32)
static void nb__cycle_connections(void) {
	int i;
	unsigned long long time_now = nb__get_time_ms_now();
	for (i = 0; i < nb__net_state->num_conn; i++) {
		nb__connection_t *c = nb__net_state->active_connections[i];
		// First check for accepts

		// We are changing the API to not fire an accept event _everytime_ 
		// a new connection is ready
/*
		if (c->accept_queue->current_elems) {
			c->callback_f(QUEUE_EVENT_ACCEPT_READY, c);
		}
*/
		// Now check for read ready
		if (c->input_queue->current_elems) {
			c->callback_f(QUEUE_EVENT_READ_READY, c);
		}
	}
}

unsigned long long nb__time_now = -1;
void nb__main_loop_step(void) {
	//nb__run_ingress_step();	
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	nb__time_now = tv.tv_sec * 1000 + tv.tv_nsec / 1000000;

	int len = 0;	

	// Something that is not NULL;
	void* p = &len;	

	// Currently just processing one packet
	p =  nb__poll_packet(&len, nb__packet_headroom);
	if (p != NULL)
		nb__run_ingress_step(p, len);

	nb__cycle_connections();
	
	nb__check_timers();
}

// Set this at init 
//char nb__my_host_id[6] = {0};
unsigned long long nb__my_host_id;

nb__net_state_t* nb__net_state;


void nb__debug_packet(char* p, size_t len) {

	int num_rows = ((len + 15) / 16);	
	for (int r = 0; r < num_rows; r++) {
		for (int c = 0; c < 16; c++) {
			if (r * 16 + c < len)
				printf("%02x ", (unsigned char) p[r * 16 + c]);
			else 
				printf("   ");
		}
		printf ("| ");
		for (int c = 0; c < 16; c++) {
			if ((r * 16 + c < len) && isprint(p[r * 16 + c])) 
				printf("%c", p[r * 16 + c]);	
			else
				printf(".");
		}
		printf("\n");
	}
}

unsigned long long nb__wildcard_host_identifier = 0;



unsigned long long nb__get_time_ms_now(void) {
	if (nb__time_now == -1) {
		struct timespec tv;
		clock_gettime(CLOCK_MONOTONIC, &tv);
		nb__time_now = tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
	}
	return nb__time_now;
}

void nb__set_user_data(nb__connection_t* c, void* user_data) {
	c->user_data = user_data;
}
void* nb__get_user_data(nb__connection_t* c) {
	return c->user_data;
}
