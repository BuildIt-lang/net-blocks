#include "modules/identifier_module.h"

namespace net_blocks {

void identifier_module::init_module(void) {
	conn_layout.register_member("dst_host_id");	
	conn_layout.register_member("dst_app_id");


	net_packet.add_member("dst_host_id", new generic_integer_member<int>((int)generic_integer_member<int>::flags::aligned));
	net_packet.add_member("src_host_id", new generic_integer_member<int>((int)generic_integer_member<int>::flags::aligned));

	net_packet.add_member("dst_app_id", new generic_integer_member<int>((int)generic_integer_member<int>::flags::aligned));
	net_packet.add_member("src_app_id", new generic_integer_member<int>((int)generic_integer_member<int>::flags::aligned));
	
	framework::instance.register_module(this);
}

module::hook_status identifier_module::hook_establish(builder::dyn_var<connection_t*> c, 
	builder::dyn_var<unsigned int> h, builder::dyn_var<unsigned int> a) {	
	
	conn_layout.get(c, "dst_host_id") = h;
	conn_layout.get(c, "dst_app_id") = a;

	return module::hook_status::HOOK_CONTINUE;
}


module::hook_status identifier_module::hook_destablish(builder::dyn_var<connection_t*> c) {
	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status identifier_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p,
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {

	net_packet["dst_host_id"]->set_integer(p, conn_layout.get(c, "dst_host_id"));
	net_packet["dst_app_id"]->set_integer(p, conn_layout.get(c, "dst_app_id"));

	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status identifier_module::hook_ingress(packet_t) {
	return module::hook_status::HOOK_CONTINUE;
}

}
