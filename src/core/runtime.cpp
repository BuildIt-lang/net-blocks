#include "core/runtime.h"

namespace net_blocks {
namespace runtime {
builder::dyn_var<void* (int)> malloc ("malloc");
builder::dyn_var<void (void*)> free ("free");
builder::dyn_var<void* (void*)> to_void_ptr("(void*)");
builder::dyn_var<void (void*, void*, int)> memcpy("memcpy");

builder::dyn_var<int (void)> connection_t_size ("nbr__connection_t_size");

builder::dyn_var<void* (int*)> poll_packet ("nbr__poll_packet");
builder::dyn_var<void (void*, int)> send_packet ("nbr__send_packet");
}
}

