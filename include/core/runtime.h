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

extern builder::dyn_var<int (void)> size_of;

extern builder::dyn_var<char* (int*, int)> poll_packet;
extern builder::dyn_var<int (char*, int)> send_packet;

extern builder::dyn_var<void* (void)> new_data_queue;
extern builder::dyn_var<void (void*)> free_data_queue;
extern builder::dyn_var<void (void*, void*, int)> insert_data_queue;

extern builder::dyn_var<char*> my_host_id;

extern builder::dyn_var<void (void*)> debug_packet;
extern builder::dyn_var<char*> reuse_mtu_buffer;

extern builder::dyn_var<char* (void)> request_send_buffer;
extern builder::dyn_var<void (char*)> return_send_buffer;


extern builder::dyn_var<void*> net_state_obj;
}

}

#endif
