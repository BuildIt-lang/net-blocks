#include "modules/signaling_module.h"
#include "builder/static_var.h"	

namespace net_blocks {

signaling_module signaling_module::instance;
signaling_module_after signaling_module_after::instance;

void signaling_module::init_module(void) {	
	framework::instance.register_module(this);	

	// Field to communicate to with the reliable delivery module
	net_packet.add_member("is_signaling", new generic_integer_member<int>((int)member_flags::aligned), 0);	
	
	conn_layout.register_member<builder::dyn_var<int>>("signaling_state");
}
void signaling_module_after::init_module(void) {	
	framework::instance.register_module(this);	
}


module::hook_status signaling_module_after::hook_establish(builder::dyn_var<connection_t*> c,
	builder::dyn_var<unsigned int> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa) {

	if (h == runtime::wildcard_host_identifier || a == 0) {	
		conn_layout.get(c, "signaling_state") = signaling_module::SIGNAL_NA;
		return module::hook_status::HOOK_CONTINUE;
	}

	if (!signaling_module::instance.is_enabled) {
		conn_layout.get(c, "signaling_state") = signaling_module::SIGNALED;
		return module::hook_status::HOOK_CONTINUE;
	}
	

	// Create and send a signalling packet
	// Signalling packet has a length 0 

	packet_t p_sig = runtime::request_send_buffer();
	// Invoke the send path from after this module	
	builder::dyn_var<char[1]> buff = {0};
	builder::dyn_var<int> ret_len;
	builder::dyn_var<int*> ret_len_ptr = &ret_len;

	
	conn_layout.get(c, "signaling_state") = signaling_module::WAITING_FOR_SIGNAL;

	if (!framework::instance.isIPCompat())
		net_packet["total_len"]->set_integer(p_sig, net_packet.get_total_size() - get_headroom() - 1);
	net_packet["computed_total_len"]->set_integer(p_sig, net_packet.get_total_size() - get_headroom() - 1);
	for (builder::static_var<unsigned int> i = signaling_module::instance.m_sequence + 1; i < framework::instance.m_registered_modules.size(); i++) {
		builder::static_var<int> s = (int)framework::instance.m_registered_modules[i]->hook_send(c, p_sig, buff, 0, ret_len_ptr);
		if (s == (int)module::hook_status::HOOK_DROP)
			break;
	}

	return module::hook_status::HOOK_CONTINUE;	
}

module::hook_status signaling_module::hook_ingress(packet_t p) {
	builder::dyn_var<connection_t*> c = runtime::to_void_ptr(net_packet["flow_identifier"]->get_integer(p));
	if (conn_layout.get(c, "signaling_state") == WAITING_FOR_SIGNAL)
		conn_layout.get(c, "signaling_state") = SIGNALED;
	return module::hook_status::HOOK_CONTINUE;	
}

}
