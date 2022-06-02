#include "modules/payload_module.h"
#include "modules/inorder_module.h"

namespace net_blocks {

payload_module payload_module::instance;

void payload_module::init_module(void) {	
	// This should always be the last member
	// TODO: introduce groups and add this member in the last group
	net_packet.add_member("payload", new generic_integer_member<char>((int)member_flags::aligned), 5);
	m_establish_depends.clear();
	m_destablish_depends.clear();
	m_send_depends.clear();
	m_ingress_depends = {&inorder_module::instance};
	framework::instance.register_module(this);	
}

module::hook_status payload_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p,
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {
	ret_len[0] = len;
	runtime::memcpy(net_packet["payload"]->get_addr(p), buff, len);	
	net_packet["total_len"]->set_integer(p, (net_packet.get_total_size() - get_headroom() - 1) + len);
	return module::hook_status::HOOK_CONTINUE;
}


}
