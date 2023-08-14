#ifndef NB_RUNTIME_H
#define NB_RUNTIME_H
#include <stdlib.h>
#include <string.h>
#include "gen_headers.h"
#include "nb_timer.h"
#include <stdio.h>
#ifdef __cplusplus 
extern "C" {
#endif

#define QUEUE_EVENT_ESTABLISHED (0)
#define QUEUE_EVENT_READ_READY (1)
#define QUEUE_EVENT_ACCEPT_READY (2)

static inline int nb__connection_t_size(void) {
	return sizeof(nb__connection_t);
}

static inline void nb__assert(int b, const char* error) {
	if (!b) {
		fprintf(stderr, "%s\n", error);
		abort();
	}
}


// Data queue methods
struct data_queue_t* nb__new_data_queue(void);
void nb__free_data_queue(struct data_queue_t*);
void nb__insert_data_queue(struct data_queue_t*, char*, int);

// Accept queue methods
nb__accept_queue_t* nb__new_accept_queue(void);
void nb__free_accept_queue(nb__accept_queue_t*);
void nb__insert_accept_queue(nb__accept_queue_t*, unsigned, char*, void*);

void nb__add_connection(nb__connection_t*, unsigned sa);
void nb__delete_connection(unsigned sa);
nb__connection_t* nb__retrieve_connection(unsigned sa);


void nb__debug_packet(char* p);


// Network interface API
char* nb__poll_packet(int*, int);
int nb__send_packet(char*, int);
void nb__ipc_init(const char* sock_path, int mode);
void nb__ipc_deinit();
void nb__mlx5_init(void);
char* nb__request_send_buffer(void);
void* nb__return_send_buffer(char*);


// Generated protocol API
void nb__run_ingress_step (void*, int);
int nb__send (nb__connection_t* arg0, char* arg1, int arg2);
void nb__destablish (nb__connection_t* arg0);
nb__connection_t* nb__establish (char* arg0, unsigned int arg1, unsigned int arg2, void (*arg3)(int, nb__connection_t*));
void nb__net_init (void);
void nb__reliable_redelivery_timer_cb(nb__timer*, void* param, unsigned long long to);

// Runtime API
int nb__read(nb__connection_t*, char*, int);
nb__connection_t* nb__accept(nb__connection_t*, void (*)(int, nb__connection_t*));

void nb__main_loop_step(void);

void nb__set_user_data(nb__connection_t*, void* user_data);
void* nb__get_user_data(nb__connection_t*);





extern char nb__reuse_mtu_buffer[];
extern char nb__my_host_id[];
extern nb__net_state_t* nb__net_state;

extern char nb__wildcard_host_identifier[];


extern unsigned long long nb__get_time_ms_now(void);
#ifdef __cplusplus 
}
#endif

#endif
