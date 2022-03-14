#include "core/runtime.h"

namespace net_blocks {
namespace runtime {
builder::dyn_var<void* (int)> malloc ("malloc");
builder::dyn_var<void (void*)> free ("free");
builder::dyn_var<void* (void*)> to_void_ptr("(void*)");
builder::dyn_var<void (void*, void*, int)> memcpy("memcpy");

builder::dyn_var<int (void)> connection_t_size ("nbr__connection_t_size");

builder::dyn_var<char* (int*)> poll_packet ("nbr__poll_packet");
builder::dyn_var<int (char*, int)> send_packet ("nbr__send_packet");

builder::dyn_var<void* (void)> new_data_queue("nbr__new_data_queue");
builder::dyn_var<void (void*)> free_data_queue("nbr__free_data_queue");
builder::dyn_var<void (void*, void*, int)> insert_data_queue("nbr__insert_data_queue");

builder::dyn_var<void (void*, int)> add_connection("nbr__add_connection");
builder::dyn_var<void (int)> delete_connection("nbr__delete_connection");
builder::dyn_var<void* (int)> retrieve_connection("nbr__retrieve_connection");

builder::dyn_var<unsigned int> my_host_id("nbr__my_host_id");
builder::dyn_var<void (void*)> debug_packet("nbr__debug_packet");
}
}

