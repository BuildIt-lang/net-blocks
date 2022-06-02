#ifndef NET_BLOCKS_SIGNALING_MODULE_H
#define NET_BLOCKS_SIGNALING_MODULE_H

#include "core/framework.h"
#include "modules/interface_module.h"

namespace net_blocks {

class signaling_module: public module {
private:
	bool use_signaling_packet = false;

public:
	void init_module(void);

	// Various path hooking routines
	module::hook_status hook_establish(builder::dyn_var<connection_t*> c, 
		builder::dyn_var<char*> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa);

	module::hook_status hook_ingress(packet_t);


	void configSignaling(bool v) {
		use_signaling_packet = v;
	}	

private:
	signaling_module() = default;
public:
	static signaling_module instance;
};
}



#endif
