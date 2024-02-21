#include "core/runtime.h"

namespace net_blocks {
namespace runtime {

builder::dyn_var<void (int, char*)> nb_assert = builder::as_global("nb__assert");

builder::dyn_var<void* (int)> malloc = builder::as_global("malloc");
builder::dyn_var<void (void*)> free  = builder::as_global("free");
builder::dyn_var<void* (void*)> to_void_ptr = builder::as_global("(void*)");
builder::dyn_var<unsigned long long (void*)> to_ull = builder::as_global("(unsigned long long)");
builder::dyn_var<void (void*, void*, int)> memcpy = builder::as_global("memcpy");
builder::dyn_var<int (void*, void*, int)> memcmp = builder::as_global("memcmp");

builder::dyn_var<void(void)> sprintf = builder::as_global("sprintf");

builder::dyn_var<int (void)> size_of = builder::as_global("sizeof");

builder::dyn_var<char* (int*, int)> poll_packet = builder::as_global ("nb__poll_packet");
builder::dyn_var<int (char*, int)> send_packet = builder::as_global ("nb__send_packet");

builder::dyn_var<void* (void)> new_data_queue = builder::as_global("nb__new_data_queue");
builder::dyn_var<void (void*)> free_data_queue = builder::as_global("nb__free_data_queue");
builder::dyn_var<void (void*, void*, int)> insert_data_queue = builder::as_global("nb__insert_data_queue");

builder::dyn_var<unsigned int> my_host_id = builder::as_global("nb__my_host_id");
builder::dyn_var<unsigned long long> my_local_host_id = builder::as_global("nb__my_local_host_id");
builder::dyn_var<unsigned int> wildcard_host_identifier = builder::as_global("nb__wildcard_host_identifier");

builder::dyn_var<void (void*)> debug_packet = builder::as_global("nb__debug_packet");
builder::dyn_var<char*> reuse_mtu_buffer = builder::as_global("nb__reuse_mtu_buffer");


builder::dyn_var<char* (void)> request_send_buffer = builder::as_global("nb__request_send_buffer");
builder::dyn_var<void (char*)> return_send_buffer = builder::as_global("nb__return_send_buffer");


builder::dyn_var<void*> net_state_obj = builder::as_global("nb__net_state");

builder::dyn_var<void* (void)> new_accept_queue = builder::as_global("nb__new_accept_queue");
builder::dyn_var<void (void*)> free_accept_queue = builder::as_global("nb__free_accept_queue");
builder::dyn_var<void (void*, void*)> insert_accept_queue = builder::as_global("nb__insert_accept_queue");

builder::dyn_var<unsigned long long(void)> get_time_ms_now = builder::as_global("nb__get_time_ms_now");

const char timer_t_name[] = "nb__timer";
builder::dyn_var<void(void)> init_timers = builder::as_global("nb__init_timers");
builder::dyn_var<timer_t*(void)> alloc_timer = builder::as_global("nb__alloc_timer");
builder::dyn_var<void(timer_t*)> return_timer = builder::as_global("nb__return_timer");
builder::dyn_var<void(timer_t*, unsigned long long, void*, void*)> insert_timer = builder::as_global("nb__insert_timer");
builder::dyn_var<void(timer_t*)> remove_timer = builder::as_global("nb__remove_timer");


const char routing_table_entry_name[] = "struct routing_table_entry";
builder::dyn_var<unsigned long long(unsigned int, rte_t*, int)> routing_lookup_from_global = 
	builder::as_global("nb__routing_table_lookup_from_global");

builder::dyn_var<unsigned short(unsigned char*, size_t)> do_ip_checksum = builder::as_global("nb__do_ip_checksum");


builder::dyn_var<unsigned short(unsigned short)> u_htons = builder::as_global("htons");
builder::dyn_var<unsigned short(unsigned short)> u_ntohs = builder::as_global("ntohs");
builder::dyn_var<void(void)> info_log = builder::as_global("nb__info_log");
}
}

