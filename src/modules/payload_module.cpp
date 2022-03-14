#include "modules/payload_module.h"

namespace net_blocks {

void payload_module::init_module(void) {	
	// This should always be the last member
	// TODO: introduce groups and add this member in the last group
	net_packet.add_member("payload", new generic_integer_member<char>((int)generic_integer_member<char>::flags::aligned));
	framework::instance.register_module(this);	
}

module::hook_status payload_module::hook_send(builder::dyn_var<connection_t*> c, packet_t, 
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {
	ret_len[0] = len;
	runtime::memcpy(net_packet["payload"]->get_addr(c), buff, len);	
	net_packet["total_len"]->set_integer(c, (net_packet.get_total_size() - 1) + len);
	return module::hook_status::HOOK_CONTINUE;
}

}
