#ifndef NET_BLOCKS_NETWORK_MODULE_H
#define NET_BLOCKS_NETWORK_MODULE_H

#include "core/framework.h"

namespace net_blocks {

class network_module: public module {
public:
	void init_module(void);
	
	// Various path hooking routines	
	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len);


private:
	network_module() = default;
public:
	static network_module instance;

};


}

#endif
