#include "modules/network_module.h"
#include "modules/checksum_module.h"

namespace net_blocks {

network_module network_module::instance;

void network_module::init_module(void) {
	m_establish_depends = {&checksum_module::instance};
	m_destablish_depends = {&checksum_module::instance};
	m_send_depends = {&checksum_module::instance};
	m_ingress_depends.clear();

	framework::instance.register_module(this);	
}


module::hook_status network_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p,
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {
	builder::dyn_var<int> size = net_packet["total_len"]->get_integer(p);
	runtime::send_packet(p + get_headroom(), size);
	//runtime::return_send_buffer(p);
	return module::hook_status::HOOK_CONTINUE;
}


}
