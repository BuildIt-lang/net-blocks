#include "modules/routing_module.h"
#include "core/packet.h"
#include <arpa/inet.h>
namespace net_blocks {

routing_module routing_module::instance;

void routing_module::init_module(void) {		

	if (!is_enabled)
		return;

	if (framework::instance.isIPCompat()) {
		net_packet.add_member("ip_VIDE", 
			new generic_integer_member<unsigned short>((int) member_flags::aligned), 2);
		net_packet.add_member("ip_total_len", 
			new generic_integer_member<unsigned short>((int) member_flags::aligned), 2);
		net_packet.add_member("ip_IFF", 
			new generic_integer_member<unsigned int>((int) member_flags::aligned), 2);
		net_packet.add_member("ip_ttl",
			new generic_integer_member<unsigned char>((int) member_flags::aligned), 2);
		net_packet.add_member("ip_protocol", 
			new generic_integer_member<unsigned char>((int) member_flags::aligned), 2);
		
		net_packet.add_member("ip_header_checksum", 
			new generic_integer_member<unsigned short>((int) member_flags::aligned), 2);
		
	}	

	auto dst_global_id = new generic_integer_member<unsigned int>((int) member_flags::aligned);
	auto src_global_id = new generic_integer_member<unsigned int>((int) member_flags::aligned);

	net_packet.add_member("dst_global_id", dst_global_id, 2);
	net_packet.add_member("src_global_id", src_global_id, 2);

	conn_layout.register_member<builder::dyn_var<unsigned int>>("dst_global_id");

	framework::instance.register_module(this);	
}

module::hook_status routing_module::hook_establish(builder::dyn_var<connection_t*> c,
	builder::dyn_var<unsigned int> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa) {
	conn_layout.get(c, "dst_global_id") = sa;
	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status routing_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p,
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {

	// Assuming routing is enabled
	net_packet["dst_global_id"]->set_integer(p, conn_layout.get(c, "dst_global_id"));	
	net_packet["src_global_id"]->set_integer(p, runtime::my_host_id);

	if (framework::instance.isIPCompat()) {
		net_packet["ip_VIDE"]->set_integer(p, 0x0045);
		net_packet["ip_IFF"]->set_integer(p, 0x0040724au);
		net_packet["ip_ttl"]->set_integer(p, 0x40);
		net_packet["ip_protocol"]->set_integer(p, 0x11); // UDP compatibility mode
		// Header checksum at the end
		net_packet["ip_header_checksum"]->set_integer(p, 0);
		// TODO: This should be computed based on the address of payload instead
		builder::dyn_var<unsigned short> ip_len = runtime::u_htons(len + 28);
		net_packet["ip_total_len"]->set_integer(p, ip_len);

		builder::dyn_var<unsigned char*> header_addr = net_packet["ip_VIDE"]->get_addr(p);
		int header_len = 20;
		
		net_packet["ip_header_checksum"]->set_integer(p, runtime::do_ip_checksum(header_addr, header_len));
	}
	
	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status routing_module::hook_ingress(packet_t p) {
	if (framework::instance.isIPCompat()) {
		// TODO: This 14 needs to be cleaned up
		builder::dyn_var<unsigned short> total_len = runtime::u_ntohs(net_packet["ip_total_len"]->get_integer(p)) + 14;
		net_packet["computed_total_len"]->set_integer(p, total_len);
		//runtime::info_log("Setting computed_len to (%d)", get_module_name(), total_len);
	}
	return module::hook_status::HOOK_CONTINUE;	
}

}
