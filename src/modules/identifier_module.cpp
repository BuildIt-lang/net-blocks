#include "modules/identifier_module.h"
#include "core/runtime.h"

namespace net_blocks {

void identifier_module::init_module(void) {
	conn_layout.register_member("dst_host_id");	
	conn_layout.register_member("dst_app_id");
	conn_layout.register_member("src_app_id");

	conn_layout.register_member("input_queue");

	net_packet.add_member("dst_host_id", new generic_integer_member<int>((int)generic_integer_member<int>::flags::aligned));
	net_packet.add_member("src_host_id", new generic_integer_member<int>((int)generic_integer_member<int>::flags::aligned));

	net_packet.add_member("dst_app_id", new generic_integer_member<int>((int)generic_integer_member<int>::flags::aligned));
	net_packet.add_member("src_app_id", new generic_integer_member<int>((int)generic_integer_member<int>::flags::aligned));
	
	
	framework::instance.register_module(this);
}

module::hook_status identifier_module::hook_establish(builder::dyn_var<connection_t*> c, 
	builder::dyn_var<unsigned int> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa) {	
	
	conn_layout.get(c, "dst_host_id") = h;
	conn_layout.get(c, "dst_app_id") = a;
	conn_layout.get(c, "src_app_id") = sa;

	conn_layout.get(c, "input_queue") = runtime::new_data_queue();
	runtime::add_connection(c, sa);	
	return module::hook_status::HOOK_CONTINUE;
}


module::hook_status identifier_module::hook_destablish(builder::dyn_var<connection_t*> c) {
	runtime::delete_connection(conn_layout.get(c, "src_app_id"));
	runtime::free_data_queue(conn_layout.get(c, "input_queue"));

	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status identifier_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p,
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {

	net_packet["dst_host_id"]->set_integer(p, conn_layout.get(c, "dst_host_id"));
	net_packet["dst_app_id"]->set_integer(p, conn_layout.get(c, "dst_app_id"));
	net_packet["src_host_id"]->set_integer(p, runtime::my_host_id);
	net_packet["src_app_id"]->set_integer(p, conn_layout.get(c, "src_app_id"));

	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status identifier_module::hook_ingress(packet_t p) {
	builder::dyn_var<int> dst_id = net_packet["dst_host_id"]->get_integer(p);

	// Filter only packets intended for this host
	if (dst_id != runtime::my_host_id)
		return module::hook_status::HOOK_DROP;
	
	// Identify connection based on target app id
	builder::dyn_var<unsigned int> dst_app_id = net_packet["dst_app_id"]->get_integer(p);
	builder::dyn_var<connection_t*> c = runtime::retrieve_connection(dst_app_id);

	if (c == 0)
		return module::hook_status::HOOK_DROP;

	builder::dyn_var<int> payload_len = net_packet["total_len"]->get_integer(p) - ((net_packet.get_total_size()) - 1);
	runtime::insert_data_queue(conn_layout.get(c, "input_queue"), net_packet["payload"]->get_addr(p), payload_len);
		
	return module::hook_status::HOOK_CONTINUE;
}

}
