#include "core/runtime.h"

namespace net_blocks {
namespace runtime {
builder::dyn_var<void* (int)> malloc ("malloc");
builder::dyn_var<void (void*)> free ("free");
builder::dyn_var<void* (void*)> to_void_ptr("(void*)");
builder::dyn_var<void (void*, void*, int)> memcpy("memcpy");
builder::dyn_var<int (void*, void*, int)> memcmp("memcmp");

builder::dyn_var<int (void)> size_of("sizeof");

builder::dyn_var<char* (int*, int)> poll_packet ("nb__poll_packet");
builder::dyn_var<int (char*, int)> send_packet ("nb__send_packet");

builder::dyn_var<void* (void)> new_data_queue("nb__new_data_queue");
builder::dyn_var<void (void*)> free_data_queue("nb__free_data_queue");
builder::dyn_var<void (void*, void*, int)> insert_data_queue("nb__insert_data_queue");

builder::dyn_var<char*> my_host_id("nb__my_host_id");
builder::dyn_var<void (void*)> debug_packet("nb__debug_packet");
builder::dyn_var<char*> reuse_mtu_buffer("nb__reuse_mtu_buffer");


builder::dyn_var<char* (void)> request_send_buffer("nb__request_send_buffer");
builder::dyn_var<void (char*)> return_send_buffer("nb__return_send_buffer");


builder::dyn_var<void*> net_state_obj("nb__net_state");

}
}

