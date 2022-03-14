#ifndef NET_BLOCKS_RUNTIME_H
#define NET_BLOCKS_RUNTIME_H
#include "builder/dyn_var.h"

namespace net_blocks {
namespace runtime {

extern builder::dyn_var<void* (int)> malloc;
extern builder::dyn_var<void (void*)> free;
extern builder::dyn_var<void (void*, void*, int)> memcpy;
extern builder::dyn_var<void* (void*)> to_void_ptr;

extern builder::dyn_var<int (void)> connection_t_size;

extern builder::dyn_var<void* (int*)> poll_packet;

}
}

#endif
