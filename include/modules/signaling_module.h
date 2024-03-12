#ifndef NET_BLOCKS_SIGNALING_MODULE_H
#define NET_BLOCKS_SIGNALING_MODULE_H

#include "core/framework.h"
#include "modules/interface_module.h"

namespace net_blocks {

class signaling_module_after: public module {

public:
	void init_module(void);

	// Various path hooking routines
	module::hook_status hook_establish(builder::dyn_var<connection_t*> c, 
		builder::dyn_var<unsigned int> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa);
public:
	static signaling_module_after instance;
	const char* get_module_name(void) override { return "SignalingModuleAfter"; }

};


class signaling_module: public module {
private:
	bool is_enabled = false;
	

public:
	void init_module(void);

	// Various path hooking routines

	module::hook_status hook_ingress(packet_t);


	void configEnableSignaling(void) {
		is_enabled = true;
	}	
	void configDisableSignaling(void) {
		is_enabled = false;
	}	

	enum signaling_state_t {
		WAITING_FOR_SIGNAL = 0,
		SIGNALED = 1,
		SIGNAL_HANDLED = 2, 
		SIGNAL_NA = 3,
	};

private:
	signaling_module() = default;
public:
	static signaling_module instance;
	const char* get_module_name(void) override { return "SignalingModule"; }
	friend class signaling_module_after;
};
}



#endif
