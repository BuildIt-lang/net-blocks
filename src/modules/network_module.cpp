#include "modules/network_module.h"

namespace net_blocks {

void network_module::init_module(void) {
	framework::instance.register_module(this);	
}


module::hook_status network_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p,
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {
	builder::dyn_var<int> size = net_packet["total_len"]->get_integer(p);
	runtime::send_packet(p, size);
	return module::hook_status::HOOK_CONTINUE;
}

}
