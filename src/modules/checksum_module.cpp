#include "modules/checksum_module.h"
#include "modules/identifier_module.h"
#include "modules/network_module.h"
#include "builder/static_var.h"

namespace net_blocks {
checksum_module checksum_module::instance;

void checksum_module::init_module(void) {	

	if (!is_enabled)
		return;

	m_establish_depends = {&identifier_module::instance};
	m_destablish_depends = {&identifier_module::instance};
	m_send_depends = {&identifier_module::instance};
	m_ingress_depends = {&network_module::instance};

	net_packet.add_member("checksum", new generic_integer_member<unsigned short>(0), 4);
	framework::instance.register_module(this);
}

static builder::dyn_var<unsigned short> compute_checksum(packet_t _p, checksum_module::checksum_type_t ct) {
	builder::dyn_var<char*> p = runtime::to_void_ptr(_p);
	// Reset the checksum first
	net_packet["checksum"]->set_integer(p, 0);
	builder::dyn_var<int> so = get_headroom();
	
	builder::dyn_var<int> eo;
	if (ct == checksum_module::full_packet) 
		eo = net_packet["computed_total_len"]->get_integer(p) + get_headroom();
	else if (ct == checksum_module::header_only)
		eo = net_packet.get_total_size() - 1;

	// Pad the bytes to be multiple of 2
	if ((eo - so) % 2) {
		p[eo] = 0;
		eo = eo + 1;
	}

	// Simple checksum by adding with wrap around
	builder::dyn_var<unsigned short> checksum = 0;
	for (builder::dyn_var<char*> ptr = p + so; ptr < p + eo; ptr = ptr + 2) {
		builder::dyn_var<unsigned short*> pp = runtime::to_void_ptr(ptr);
		checksum = checksum + pp[0];
	}
	return checksum;
}

module::hook_status checksum_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p, 
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {

	builder::dyn_var<unsigned short> checksum = compute_checksum(p, checksum_type);
	net_packet["checksum"]->set_integer(p, checksum);
	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status checksum_module::hook_ingress(packet_t p) {
	// Save the checksum
	builder::dyn_var<unsigned short> checksum_saved = net_packet["checksum"]->get_integer(p);
	
	builder::dyn_var<unsigned short> checksum = compute_checksum(p, checksum_type);
	// Put the old checksum back in case this packet has to be redelivered
	net_packet["checksum"]->set_integer(p, checksum_saved);
	
	if (checksum == checksum_saved)
		return module::hook_status::HOOK_CONTINUE;
	return module::hook_status::HOOK_DROP;
}


}
