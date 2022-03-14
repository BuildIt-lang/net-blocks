#ifndef NET_BLOCKS_MODULE_H
#define NET_BLOCKS_MODULE_H
#include "core/connection.h"
#include "core/packet.h"

namespace net_blocks {

class module {

public: 
	enum class hook_status {
		HOOK_CONTINUE,
		HOOK_DROP
	};

	virtual void init_module(void) = 0;
	
	// Various path hooking routines
	virtual hook_status hook_establish(builder::dyn_var<connection_t*> c, 
		builder::dyn_var<unsigned int>, builder::dyn_var<unsigned int>) {
		return hook_status::HOOK_CONTINUE;	
	}
	
	virtual hook_status hook_destablish(builder::dyn_var<connection_t*> c) {
		return hook_status::HOOK_CONTINUE;	
	}
	

	virtual hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {
		return hook_status::HOOK_CONTINUE;	
	}
	

	virtual hook_status hook_ingress(packet_t) {
		return hook_status::HOOK_CONTINUE;	
	}

	
	// Enable virtual inheritance
	virtual ~module();
};
}
#endif
