#include "modules/identifier_module.h"
#include "core/runtime.h"
#include <arpa/inet.h>
namespace net_blocks {

const char data_queue_t_name[] = "nb__data_queue_t";
#define MAX_ACTIVE_CONNECTIONS (512)

void identifier_module::init_module(void) {
	conn_layout.register_member<builder::dyn_var<char[HOST_IDENTIFIER_LEN]>>("dst_host_id");	
	conn_layout.register_member<builder::dyn_var<unsigned int>>("dst_app_id");
	conn_layout.register_member<builder::dyn_var<unsigned int>>("src_app_id");

	conn_layout.register_member<builder::dyn_var<data_queue_t*>>("input_queue");

	net_packet.add_member("dst_host_id", new byte_array_member<HOST_IDENTIFIER_LEN>((int)member_flags::aligned), 0);
	net_packet.add_member("src_host_id", new byte_array_member<HOST_IDENTIFIER_LEN>((int)member_flags::aligned), 0);
	// Member to identify protocol 
	net_packet.add_member("protocol_identifier", new generic_integer_member<unsigned short>((int)member_flags::aligned), 0);

	net_packet.add_member("dst_app_id", new generic_integer_member<int>((int)member_flags::aligned), 2);
	net_packet.add_member("src_app_id", new generic_integer_member<int>((int)member_flags::aligned), 2);

	net_state.register_member<builder::dyn_var<int>>("num_conn");
	net_state.register_member<builder::dyn_var<unsigned short[MAX_ACTIVE_CONNECTIONS]>>("active_dst_app_ids");
	net_state.register_member<builder::dyn_var<connection_t*[MAX_ACTIVE_CONNECTIONS]>>("active_connections");	
	
	framework::instance.register_module(this);
}

void identifier_module::hook_net_init(void) {
	net_state.get(runtime::net_state_obj, "num_conn") = 0;
}

static void add_connection(builder::dyn_var<connection_t*> c, builder::dyn_var<unsigned int> appid) {
	builder::dyn_var<int> idx = net_state.get(runtime::net_state_obj, "num_conn");
	net_state.get(runtime::net_state_obj, "num_conn") = idx + 1;
	
	net_state.get(runtime::net_state_obj, "active_dst_app_ids")[idx] = appid;
	net_state.get(runtime::net_state_obj, "active_connections")[idx] = c;
}

static builder::dyn_var<connection_t*> retrieve_connection(builder::dyn_var<unsigned int> appid) {
	builder::dyn_var<connection_t*> c = 0;
	builder::dyn_var<int> total = net_state.get(runtime::net_state_obj, "num_conn");
	for (builder::dyn_var<int> i = 0; i < total; i = i + 1) {
		if (net_state.get(runtime::net_state_obj, "active_dst_app_ids")[i] == appid) {
			c = net_state.get(runtime::net_state_obj, "active_connections")[i];
			break;
		}
	}
	return c;
}
static void delete_connection(builder::dyn_var<unsigned int> appid) {
	builder::dyn_var<int> total = net_state.get(runtime::net_state_obj, "num_conn");
	for (builder::dyn_var<int> i = 0; i < total; i = i + 1) {
		if (net_state.get(runtime::net_state_obj, "active_dst_app_ids")[i] == appid) {
			total = total - 1;
			net_state.get(runtime::net_state_obj, "num_conn") = total;
			net_state.get(runtime::net_state_obj, "active_dst_app_ids")[i] = net_state.get(runtime::net_state_obj, "active_dst_app_ids")[			total];
			net_state.get(runtime::net_state_obj, "active_connections")[i] = net_state.get(runtime::net_state_obj, "active_connections")[			total];
			break;
		}
	}
}
module::hook_status identifier_module::hook_establish(builder::dyn_var<connection_t*> c, 
	builder::dyn_var<char*> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa) {	
	
	runtime::memcpy(conn_layout.get(c, "dst_host_id"), h, HOST_IDENTIFIER_LEN);
	conn_layout.get(c, "dst_app_id") = a;
	conn_layout.get(c, "src_app_id") = sa;

	conn_layout.get(c, "input_queue") = runtime::new_data_queue();
	add_connection(c, sa);	
	return module::hook_status::HOOK_CONTINUE;
}


module::hook_status identifier_module::hook_destablish(builder::dyn_var<connection_t*> c) {
	delete_connection(conn_layout.get(c, "src_app_id"));		
	runtime::free_data_queue(conn_layout.get(c, "input_queue"));

	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status identifier_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p,
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {

	runtime::memcpy(net_packet["dst_host_id"]->get_addr(p), conn_layout.get(c, "dst_host_id"), HOST_IDENTIFIER_LEN);
	net_packet["dst_app_id"]->set_integer(p, conn_layout.get(c, "dst_app_id"));
	runtime::memcpy(net_packet["src_host_id"]->get_addr(p), runtime::my_host_id, HOST_IDENTIFIER_LEN);
	net_packet["src_app_id"]->set_integer(p, conn_layout.get(c, "src_app_id"));

	net_packet["protocol_identifier"]->set_integer(p, htons(0x0800));
	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status identifier_module::hook_ingress(packet_t p) {
	builder::dyn_var<char*> dst_id = net_packet["dst_host_id"]->get_addr(p);

	if (runtime::memcmp(dst_id, runtime::my_host_id, HOST_IDENTIFIER_LEN) != 0)
		return module::hook_status::HOOK_DROP;

	
	// Identify connection based on target app id
	builder::dyn_var<unsigned int> dst_app_id = net_packet["dst_app_id"]->get_integer(p);
	builder::dyn_var<connection_t*> c = retrieve_connection(dst_app_id);

	if (c == 0)
		return module::hook_status::HOOK_DROP;

	builder::dyn_var<int> payload_len = net_packet["total_len"]->get_integer(p) - ((net_packet.get_total_size()) - 1);
	runtime::insert_data_queue(conn_layout.get(c, "input_queue"), net_packet["payload"]->get_addr(p), payload_len);
		
	return module::hook_status::HOOK_CONTINUE;
}

}
