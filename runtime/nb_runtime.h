#ifndef NB_RUNTIME_H
#define NB_RUNTIME_H
#include <stdlib.h>
#include <string.h>
#include "gen_headers.h"
#ifdef __cplusplus 
extern "C" {
#endif


#define QUEUE_EVENT_READ_READY (0)

static int nb__connection_t_size(void) {
	return sizeof(nb__connection_t);
}

struct data_queue_t* nb__new_data_queue(void);
void nb__free_data_queue(struct data_queue_t*);
void nb__insert_data_queue(struct data_queue_t*, char*, int);

void nb__add_connection(nb__connection_t*, unsigned sa);
void nb__delete_connection(unsigned sa);
nb__connection_t* nb__retrieve_connection(unsigned sa);


void nb__debug_packet(char* p);


// Network interface API
char* nb__poll_packet(int*, int);
int nb__send_packet(char*, int);
void nb__ipc_init(const char* sock_path, int mode);
void nb__mlx5_init(void);
char* nb__request_send_buffer(void);
void* nb__return_send_buffer(char*);


// Generated protocol API
void nb__run_ingress_step (void);
int nb__send (nb__connection_t* arg0, char* arg1, int arg2);
void nb__destablish (nb__connection_t* arg0);
nb__connection_t* nb__establish (char* arg0, unsigned int arg1, unsigned int arg2, void (*arg3)(int, nb__connection_t*));
void nb__net_init (void);

// Runtime API
int nb__read(nb__connection_t*, char*, int);
void nb__main_loop_step(void);





extern char nb__reuse_mtu_buffer[];
extern char nb__my_host_id[];
extern nb__net_state_t* nb__net_state;
#ifdef __cplusplus 
}
#endif

#endif
