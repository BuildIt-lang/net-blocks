#ifndef NET_BLOCKS_RUNTIME_H
#define NET_BLOCKS_RUNTIME_H
#include "builder/dyn_var.h"

namespace net_blocks {
namespace runtime {

extern builder::dyn_var<void (int, char*)> nb_assert;

extern builder::dyn_var<void* (int)> malloc;
extern builder::dyn_var<void (void*)> free;
extern builder::dyn_var<void (void*, void*, int)> memcpy;
extern builder::dyn_var<int (void*, void*, int)> memcmp;
extern builder::dyn_var<void* (void*)> to_void_ptr;
extern builder::dyn_var<unsigned long long (void*)> to_ull;

extern builder::dyn_var<void(void)> sprintf;

extern builder::dyn_var<int (void)> size_of;

extern builder::dyn_var<char* (int*, int)> poll_packet;
extern builder::dyn_var<int (char*, int)> send_packet;

extern builder::dyn_var<void* (void)> new_data_queue;
extern builder::dyn_var<void (void*)> free_data_queue;
extern builder::dyn_var<void (void*, void*, int)> insert_data_queue;

extern builder::dyn_var<void* (void)> new_accept_queue;
extern builder::dyn_var<void (void*)> free_accept_queue;
extern builder::dyn_var<void (void*, void*)> insert_accept_queue;

extern builder::dyn_var<unsigned int> my_host_id;
extern builder::dyn_var<unsigned long long> my_local_host_id;

extern builder::dyn_var<void (void*)> debug_packet;
extern builder::dyn_var<char*> reuse_mtu_buffer;

extern builder::dyn_var<char* (void)> request_send_buffer;
extern builder::dyn_var<void (char*)> return_send_buffer;


extern builder::dyn_var<void*> net_state_obj;

extern builder::dyn_var<unsigned int> wildcard_host_identifier;

// timer related functions
extern const char timer_t_name[];
using timer_t = builder::name<timer_t_name>;

extern builder::dyn_var<void(void)> init_timers;
extern builder::dyn_var<timer_t*(void)> alloc_timer;
extern builder::dyn_var<void(timer_t*)> return_timer;
extern builder::dyn_var<void(timer_t*, unsigned long long, void*, void*)> insert_timer;
extern builder::dyn_var<void(timer_t*)> remove_timer;

// Routing table related functions
extern const char routing_table_entry_name[];
using rte_t = builder::name<routing_table_entry_name>;

extern builder::dyn_var<unsigned long long(unsigned int, rte_t*, int)> routing_lookup_from_global;
extern builder::dyn_var<unsigned short(unsigned char*, size_t)> do_ip_checksum;

// general utility functions
extern builder::dyn_var<unsigned long long(void)> get_time_ms_now;
extern builder::dyn_var<unsigned short(unsigned short)> u_htons;
extern builder::dyn_var<unsigned short(unsigned short)> u_ntohs;
extern builder::dyn_var<void(void)> info_log;
}
}

#endif
