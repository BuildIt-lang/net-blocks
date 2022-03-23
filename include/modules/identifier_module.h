#ifndef NET_BLOCKS_IDENTIFIER_MODULE_H
#define NET_BLOCKS_IDENTIFIER_MODULE_H

#include "core/framework.h"

namespace net_blocks {

#define HOST_IDENTIFIER_LEN (6)

class identifier_module: public module {
public:
	void init_module(void);
	
	// Various path hooking routines
	module::hook_status hook_establish(builder::dyn_var<connection_t*> c, 
		builder::dyn_var<char*> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa);
	
	module::hook_status hook_destablish(builder::dyn_var<connection_t*> c);	

	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len);

	module::hook_status hook_ingress(packet_t);
};

}


#endif
