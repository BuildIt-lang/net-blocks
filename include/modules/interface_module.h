#ifndef NET_BLOCKS_INTERFACE_MODULE_H
#define NET_BLOCKS_INTERFACE_MODULE_H

#include "core/framework.h"

// Should be the same as the defines 
// in runtime/nb_runtime.h
#define QUEUE_EVENT_ESTABLISHED (0)
#define QUEUE_EVENT_READ_READY (1)
#define QUEUE_EVENT_ACCEPT_READY (2)

namespace net_blocks {

typedef builder::dyn_var<void (int, connection_t*)> callback_t;

// Interface module is a special module that doesn't actually 
// hook any paths
class interface_module {

	interface_module() = default;

public:
	static interface_module instance;

	void init_module(void);
	
	builder::dyn_var<connection_t*> establish_impl(builder::dyn_var<char*>, builder::dyn_var<unsigned int>, 
		builder::dyn_var<unsigned int> ca, callback_t);

	void destablish_impl(builder::dyn_var<connection_t*>);

	builder::dyn_var<int> send_impl(builder::dyn_var<connection_t*>, builder::dyn_var<char*>, builder::dyn_var<int>);
	
	// This function is invoked when a packet is polled, 
	// runs the ingress path
	void run_ingress_step(builder::dyn_var<void*>, builder::dyn_var<int>);

	void net_init_impl(void);


};

}

#endif
