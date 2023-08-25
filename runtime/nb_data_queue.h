#ifndef NB_DATA_QUEUE_H
#define NB_DATA_QUEUE_H

#define MAX_DATA_QUEUE_ELEMS (512)
#define MAX_ACCEPT_QUEUE_ELEMS (512)
#define HOST_IDENTIFIER_LEN (6)

struct data_queue_t {
	char* data_queue_elems[MAX_DATA_QUEUE_ELEMS];
	int data_queue_elems_size[MAX_DATA_QUEUE_ELEMS];
	int current_elems;	
};
typedef struct data_queue_t nb__data_queue_t;


struct accept_queue_t {
	unsigned src_app_id[MAX_ACCEPT_QUEUE_ELEMS];
	unsigned long long src_host_id[MAX_ACCEPT_QUEUE_ELEMS];	
	void* packet[MAX_ACCEPT_QUEUE_ELEMS];
	int current_elems;	
};
typedef struct accept_queue_t nb__accept_queue_t;

#endif
