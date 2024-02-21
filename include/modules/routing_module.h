#ifndef NET_BLOCKS_ROUTING_MODULE_H
#define NET_BLOCKS_ROUTING_MODULE_H

#include "core/framework.h"
#include "modules/interface_module.h"

namespace net_blocks {

class routing_module: public module {
private:
	bool is_enabled = true;
public:
	void init_module(void);

	// Various path hooking routines
	module::hook_status hook_establish(builder::dyn_var<connection_t*> c, 
		builder::dyn_var<unsigned int> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa);

	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t p,
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len);

	module::hook_status hook_ingress(packet_t);

	void configEnableRouting(void) {
		is_enabled = true;
	}
	void configDisableRouting() {
		is_enabled = false;
	}
private:
	routing_module() = default;
public:
	static routing_module instance;
	const char* get_module_name(void) override { return "RoutingModule"; }
};
}



#endif
