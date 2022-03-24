#ifndef NB_DATA_QUEUE_H
#define NB_DATA_QUEUE_H

#define MAX_DATA_QUEUE_ELEMS (512)

struct data_queue_t {
	char* data_queue_elems[MAX_DATA_QUEUE_ELEMS];
	int data_queue_elems_size[MAX_DATA_QUEUE_ELEMS];
	int current_elems;	
};
typedef struct data_queue_t nb__data_queue_t;

#endif
