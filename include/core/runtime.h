#ifndef NET_BLOCKS_RUNTIME_H
#define NET_BLOCKS_RUNTIME_H
#include "builder/dyn_var.h"

namespace net_blocks {
namespace runtime {

extern builder::dyn_var<void* (int)> malloc;
extern builder::dyn_var<void (void*)> free;
extern builder::dyn_var<void (void*, void*, int)> memcpy;
extern builder::dyn_var<int (void*, void*, int)> memcmp;
extern builder::dyn_var<void* (void*)> to_void_ptr;
extern builder::dyn_var<unsigned long long (void*)> to_ull;

extern builder::dyn_var<int (void)> size_of;

extern builder::dyn_var<char* (int*, int)> poll_packet;
extern builder::dyn_var<int (char*, int)> send_packet;

extern builder::dyn_var<void* (void)> new_data_queue;
extern builder::dyn_var<void (void*)> free_data_queue;
extern builder::dyn_var<void (void*, void*, int)> insert_data_queue;

extern builder::dyn_var<void* (void)> new_accept_queue;
extern builder::dyn_var<void (void*)> free_accept_queue;
extern builder::dyn_var<void (void*, void*)> insert_accept_queue;

extern builder::dyn_var<char*> my_host_id;

extern builder::dyn_var<void (void*)> debug_packet;
extern builder::dyn_var<char*> reuse_mtu_buffer;

extern builder::dyn_var<char* (void)> request_send_buffer;
extern builder::dyn_var<void (char*)> return_send_buffer;


extern builder::dyn_var<void*> net_state_obj;
extern builder::dyn_var<unsigned long long(void)> get_time_ms_now;


// timer related functions
extern const char timer_t_name[];
using timer_t = builder::name<timer_t_name>;

extern builder::dyn_var<void(void)> init_timers;
extern builder::dyn_var<timer_t*(void)> alloc_timer;
extern builder::dyn_var<void(timer_t*)> return_timer;
extern builder::dyn_var<void(timer_t*, unsigned long long, void*, void*)> insert_timer;
extern builder::dyn_var<void(timer_t*)> remove_timer;


}


}

#endif
