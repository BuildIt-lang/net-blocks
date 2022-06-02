#include "modules/signaling_module.h"

namespace net_blocks {

signaling_module signaling_module::instance;

void signaling_module::init_module(void) {	
	framework::instance.register_module(this);	

	// Field to communicate to with the reliable delivery module
	net_packet.add_member("is_signaling", new generic_integer_member<int>((int)member_flags::aligned), 0);	
}

module::hook_status signaling_module::hook_establish(builder::dyn_var<connection_t*> c,
	builder::dyn_var<char*> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa) {
	
	if (!use_signaling_packet) {
		conn_layout.get(c, "callback_f")(QUEUE_EVENT_ESTABLISHED, c);	
	} else {
	}
	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status signaling_module::hook_ingress(packet_t) {
	return module::hook_status::HOOK_CONTINUE;	
}

}
