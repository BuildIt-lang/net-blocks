#include "core/runtime.h"

namespace net_blocks {
namespace runtime {
builder::dyn_var<void* (int)> malloc ("malloc");
builder::dyn_var<void (void*)> free ("free");
builder::dyn_var<void* (void*)> to_void_ptr("(void*)");
builder::dyn_var<void (void*, void*, int)> memcpy("memcpy");

builder::dyn_var<int (void)> connection_t_size ("nb__connection_t_size");

builder::dyn_var<char* (int*)> poll_packet ("nb__poll_packet");
builder::dyn_var<int (char*, int)> send_packet ("nb__send_packet");

builder::dyn_var<void* (void)> new_data_queue("nb__new_data_queue");
builder::dyn_var<void (void*)> free_data_queue("nb__free_data_queue");
builder::dyn_var<void (void*, void*, int)> insert_data_queue("nb__insert_data_queue");

builder::dyn_var<void (void*, int)> add_connection("nb__add_connection");
builder::dyn_var<void (int)> delete_connection("nb__delete_connection");
builder::dyn_var<void* (int)> retrieve_connection("nb__retrieve_connection");

builder::dyn_var<unsigned int> my_host_id("nb__my_host_id");
builder::dyn_var<void (void*)> debug_packet("nb__debug_packet");
}
}

