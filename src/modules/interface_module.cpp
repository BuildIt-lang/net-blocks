#include "modules/interface_module.h"
#include "core/connection.h"
#include "core/framework.h"
#include "core/runtime.h"

namespace net_blocks {

interface_module interface_module::instance;

void interface_module::init_module(void) {	
	conn_layout.register_member<callback_t>("callback_f");	
	net_packet.add_member("total_len", new generic_integer_member<int>((int)member_flags::aligned), 1);
}

builder::dyn_var<connection_t*> interface_module::establish_impl(builder::dyn_var<char*> host_id, builder::dyn_var<unsigned int> app_id, 
	builder::dyn_var<unsigned int> ca, callback_t callback) {

	builder::dyn_var<connection_t*> c = framework::instance.run_establish_path(host_id, app_id, ca);
	conn_layout.get(c, "callback_f") = callback;
		
	return c;	
}

void interface_module::destablish_impl(builder::dyn_var<connection_t*> c) {
	framework::instance.run_destablish_path(c);
}

builder::dyn_var<int> interface_module::send_impl(builder::dyn_var<connection_t*> c, builder::dyn_var<char*> buff, builder::dyn_var<int> len) {
	return framework::instance.run_send_path(c, buff, len);
}

// This function tries to poll a packet and if it finds one, 
// runs the ingress path
void interface_module::run_ingress_step(void) {
	builder::dyn_var<int> len;
	builder::dyn_var<int*> len_addr = &len;

	packet_t p = runtime::poll_packet(len_addr, 0);

	if (p != 0) 
		framework::instance.run_ingress_path(p);

}

void interface_module::net_init_impl(void) {
	framework::instance.run_net_init_path();
}

}
