#ifndef NB_RUNTIME_H
#define NB_RUNTIME_H
#include <stdlib.h>
#include <string.h>

#define QUEUE_EVENT_READ_READY (0)

#define MAX_DATA_QUEUE_ELEMS (512)

struct data_queue_t {
	char* data_queue_elems[MAX_DATA_QUEUE_ELEMS];
	int data_queue_elems_size[MAX_DATA_QUEUE_ELEMS];
	int current_elems;	
};

// This is currently hand written
// TODO: Generate this from the dynamic_object
struct connection_t {
	unsigned int dst_host_id;
	unsigned int dst_app_id;
	unsigned int src_app_id;
	
	struct data_queue_t* input_queue;
	void (*callback_f)(int, struct connection_t*);
};

typedef struct connection_t nb__connection_t;
static int nb__connection_t_size(void) {
	return sizeof(nb__connection_t);
}

struct data_queue_t* nb__new_data_queue(void);
void nb__free_data_queue(struct data_queue_t*);
void nb__insert_data_queue(struct data_queue_t*, char*, int);

void nb__add_connection(nb__connection_t*, unsigned sa);
void nb__delete_connection(unsigned sa);
nb__connection_t* nb__retrieve_connection(unsigned sa);

extern unsigned int nb__my_host_id;


char* nb__poll_packet(int*);
int nb__send_packet(char*, int);

void nb__ipc_init(const char* sock_path, int mode);

void nb__main_loop_step(void);
void nb__run_ingress_step (void);

int nb__send (nb__connection_t* arg0, char* arg1, int arg2);
void nb__destablish (nb__connection_t* arg0);
int nb__read(nb__connection_t*, char*, int);

nb__connection_t* nb__establish (unsigned int arg0, unsigned int arg1, unsigned int arg2, void (*arg3)(int, nb__connection_t*));

void nb__debug_packet(char* p);
#endif
