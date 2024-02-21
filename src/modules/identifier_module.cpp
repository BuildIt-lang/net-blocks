#include "modules/identifier_module.h"
#include "modules/reliable_module.h"
#include "modules/checksum_module.h"

#include "core/runtime.h"
#include <arpa/inet.h>
namespace net_blocks {
const char data_queue_t_name[] = "nb__data_queue_t";
const char accept_queue_t_name[] = "nb__accept_queue_t";
#define MAX_ACTIVE_CONNECTIONS (512)
namespace runtime {
builder::dyn_var<char[HOST_IDENTIFIER_LEN]> wildcard_host_identifier = builder::as_global("nb__wildcard_host_identifier");
}

identifier_module identifier_module::instance;

#define QUEUE_EVENT_ESTABLISHED (0)
#define QUEUE_EVENT_READ_READY (1)
#define QUEUE_EVENT_ACCEPT_READY (2)


void identifier_module::init_module(void) {
	//conn_layout.register_member<builder::dyn_var<char[HOST_IDENTIFIER_LEN]>>("remote_host_id");	
	conn_layout.register_member<builder::dyn_var<unsigned long long>>("remote_host_id");
	conn_layout.register_member<builder::dyn_var<unsigned int>>("remote_app_id");
	conn_layout.register_member<builder::dyn_var<unsigned int>>("local_app_id");

	conn_layout.register_member<builder::dyn_var<data_queue_t*>>("input_queue");
	conn_layout.register_member<builder::dyn_var<accept_queue_t*>>("accept_queue");

	// A non-send member
	// Network module ignores this while sending and adds headroom for this while receiving
	net_packet.add_member("flow_identifier", new generic_integer_member<unsigned long long>((int)member_flags::aligned), 0);

	auto dst_host_id = new generic_integer_member<unsigned long long, HOST_IDENTIFIER_LEN * byte_size>((int) member_flags::aligned);
	auto src_host_id = new generic_integer_member<unsigned long long, HOST_IDENTIFIER_LEN * byte_size>((int) member_flags::aligned);

	net_packet.add_member("dst_host_id", dst_host_id, 1);
	net_packet.add_member("src_host_id", src_host_id, 1);
	// Member to identify protocol 
	net_packet.add_member("protocol_identifier", new generic_integer_member<unsigned int>((int)member_flags::aligned), 1);

	auto dst_app_id = new generic_integer_member<int>((int)member_flags::aligned);
	dst_app_id->set_custom_range(8080, 8088);
	net_packet.add_member("dst_app_id", dst_app_id, 3);
	auto src_app_id = new generic_integer_member<int>(0);
	src_app_id->set_custom_range(8080, 8088);
	net_packet.add_member("src_app_id", src_app_id, 3);


	
	net_state.register_member<builder::dyn_var<int>>("num_conn");
	net_state.register_member<builder::dyn_var<unsigned int[MAX_ACTIVE_CONNECTIONS]>>("active_local_app_ids");
	if (flow_identifier == flow_identifier_t::src_dst_identifier) {
		net_state.register_member<builder::dyn_var<unsigned int[MAX_ACTIVE_CONNECTIONS]>>("active_remote_app_ids");	
		net_state.register_member<builder::dyn_var<unsigned long long[MAX_ACTIVE_CONNECTIONS]>>("active_remote_host_ids");
	}	
	net_state.register_member<builder::dyn_var<connection_t*[MAX_ACTIVE_CONNECTIONS]>>("active_connections");	

	m_establish_depends = {&reliable_module::instance};
	m_destablish_depends = {&reliable_module::instance};
	m_send_depends = {&reliable_module::instance};
	m_ingress_depends = {&checksum_module::instance};

	framework::instance.register_module(this);
}

void identifier_module::hook_net_init(void) {
	get_state("num_conn") = 0;
}

void identifier_module::add_connection(builder::dyn_var<connection_t*> c, builder::dyn_var<unsigned int> appid, 
		builder::dyn_var<unsigned int> src_app_id, builder::dyn_var<unsigned long long> src_host_id) {
	builder::dyn_var<int> idx = get_state("num_conn");
	get_state("num_conn") = idx + 1;
	
	get_state("active_local_app_ids")[idx] = appid;
	get_state("active_connections")[idx] = c;
	if (flow_identifier == flow_identifier_t::src_dst_identifier) {
		get_state("active_remote_app_ids")[idx] = src_app_id;
		//runtime::memcpy((get_state("active_remote_host_ids")[idx]), src_host_id, HOST_IDENTIFIER_LEN);
		get_state("active_remote_host_ids")[idx] = src_host_id;
	}
}

bool identifier_module::match_connection(builder::dyn_var<int> &index, builder::dyn_var<unsigned int> dst_app_id, 
	builder::dyn_var<unsigned int> src_app_id, builder::dyn_var<unsigned long long> src_host_id) {
	if (flow_identifier == flow_identifier_t::src_dst_identifier) {
		return (bool) (get_state("active_local_app_ids")[index] == dst_app_id 
				&& get_state("active_remote_app_ids")[index] == src_app_id
				&& get_state("active_remote_host_ids")[index] == src_host_id);
				//&& runtime::memcmp((get_state("active_remote_host_ids")[index]), src_host_id, 
					//HOST_IDENTIFIER_LEN) == 0);
	} else {
		return (bool) (get_state("active_local_app_ids")[index] == dst_app_id);
	}
}

builder::dyn_var<connection_t*> identifier_module::retrieve_connection(builder::dyn_var<unsigned int> appid,
		builder::dyn_var<unsigned int> src_app_id, builder::dyn_var<unsigned long long> src_host_id) {
	builder::dyn_var<connection_t*> c = 0;
	builder::dyn_var<int> total = get_state("num_conn");
	for (builder::dyn_var<int> i = 0; i < total; i = i + 1) {
		if (match_connection(i, appid, src_app_id, src_host_id)) {
			c = get_state("active_connections")[i];
			break;
		}
	}
	return c;
}

builder::dyn_var<connection_t*> identifier_module::retrieve_wildcard_connection(builder::dyn_var<unsigned int> appid) {
	// A wildcard connection is defined as the first partial match we can find
	// The only wildcard pattern we allow is dst_app_id is set and the src_app_id and src_host_id are both wildcard
	builder::dyn_var<connection_t*> c = 0;
	builder::dyn_var<int> total = get_state("num_conn");
	for (builder::dyn_var<int> i = 0; i < total; i = i + 1) {
		if (get_state("active_local_app_ids")[i] == appid
			&& get_state("active_remote_app_ids")[i] == WILDCARD_APP_ID
			//&& runtime::memcmp((get_state("active_remote_host_ids")[i]), runtime::wildcard_host_identifier, 
				//HOST_IDENTIFIER_LEN) == 0) {
			&& get_state("active_remote_host_ids")[i] == runtime::wildcard_host_identifier) {
			c = get_state("active_connections")[i];
			break;
		}
	}
	return c;
}



void identifier_module::delete_connection(builder::dyn_var<unsigned int> appid,
		builder::dyn_var<unsigned int> src_app_id, builder::dyn_var<unsigned long long> src_host_id) {

	builder::dyn_var<int> total = get_state("num_conn");
	
	for (builder::dyn_var<int> i = 0; i < total; i = i + 1) {
		if (match_connection(i, appid, src_app_id, src_host_id)) {
			total = total - 1;
			get_state("num_conn") = total;
			get_state("active_local_app_ids")[i] = get_state("active_local_app_ids")[total];
			get_state("active_connections")[i] = get_state("active_connections")[total];
			if (flow_identifier == flow_identifier_t::src_dst_identifier) {
				get_state("active_remote_app_ids")[i] = get_state("active_remote_app_ids")[total];
				//runtime::memcpy((get_state("active_remote_host_ids")[total]), (get_state("active_remote_host_ids")[i]), HOST_IDENTIFIER_LEN);
				get_state("active_remote_host_ids")[total] = get_state("active_remote_host_ids")[i];
			}
			break;
		}
	}
}
module::hook_status identifier_module::hook_establish(builder::dyn_var<connection_t*> c, 
	builder::dyn_var<unsigned long long> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa) {	
	
	//runtime::memcpy(conn_layout.get(c, "remote_host_id"), h, HOST_IDENTIFIER_LEN);
	conn_layout.get(c, "remote_host_id") = h;
	conn_layout.get(c, "remote_app_id") = a;
	conn_layout.get(c, "local_app_id") = sa;

	conn_layout.get(c, "input_queue") = runtime::new_data_queue();
	conn_layout.get(c, "accept_queue") = runtime::new_accept_queue();
	add_connection(c, sa, a, h);
	return module::hook_status::HOOK_CONTINUE;
}


module::hook_status identifier_module::hook_destablish(builder::dyn_var<connection_t*> c) {
	delete_connection(conn_layout.get(c, "local_app_id"), conn_layout.get(c, "remote_app_id"), 
		conn_layout.get(c, "remote_host_id"));		
	runtime::free_data_queue(conn_layout.get(c, "input_queue"));
	runtime::free_accept_queue(conn_layout.get(c, "accept_queue"));

	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status identifier_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p,
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {

	//runtime::memcpy(net_packet["dst_host_id"]->get_addr(p), conn_layout.get(c, "remote_host_id"), HOST_IDENTIFIER_LEN);

	net_packet["dst_host_id"]->set_integer(p, conn_layout.get(c, "remote_host_id"));
	net_packet["dst_app_id"]->set_integer(p, conn_layout.get(c, "remote_app_id"));
	//runtime::memcpy(net_packet["src_host_id"]->get_addr(p), runtime::my_host_id, HOST_IDENTIFIER_LEN);
	net_packet["src_host_id"]->set_integer(p, runtime::my_host_id);
	net_packet["src_app_id"]->set_integer(p, conn_layout.get(c, "local_app_id"));

	net_packet["protocol_identifier"]->set_integer(p, htons(0x0800));
	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status identifier_module::hook_ingress(packet_t p) {
/*
	builder::dyn_var<char*> dst_id = net_packet["dst_host_id"]->get_addr(p);

	if (runtime::memcmp(dst_id, runtime::my_host_id, HOST_IDENTIFIER_LEN) != 0)
		return module::hook_status::HOOK_DROP;
*/
	if (net_packet["dst_host_id"]->get_integer(p) != runtime::my_host_id)
		return module::hook_status::HOOK_DROP;
	
	// Identify connection based on target app id
	builder::dyn_var<unsigned int> dst_app_id = net_packet["dst_app_id"]->get_integer(p);
	builder::dyn_var<unsigned int> src_app_id = net_packet["src_app_id"]->get_integer(p);
	builder::dyn_var<unsigned long long> src_host_id = net_packet["src_host_id"]->get_integer(p);
	builder::dyn_var<connection_t*> c = retrieve_connection(dst_app_id, src_app_id, src_host_id);


	if (c != 0) {
		net_packet["flow_identifier"]->set_integer(p, runtime::to_ull(c));
		return module::hook_status::HOOK_CONTINUE;
	}


	// If we didn't find an exact match and the implementation uses src-dst identifiers, 
	// check for a wildcard match
	if (flow_identifier != flow_identifier_t::src_dst_identifier) 
		return module::hook_status::HOOK_DROP;

	
	c = retrieve_wildcard_connection(dst_app_id);
		
	
	if (c != 0) {
		runtime::insert_accept_queue(conn_layout.get(c, "accept_queue"), src_app_id, src_host_id, p);
		// At this point we will fire an accept event
		conn_layout.get(c, "callback_f")(QUEUE_EVENT_ACCEPT_READY, c);
	}
	
	runtime::nb_assert(false, "Failed to lookup connection");
	
	// It is okay to drop the packet even if we have a wildcard match, the packet will be reprocessed again
	// when the establish path is run for the new packet
	return module::hook_status::HOOK_DROP;

}


}
