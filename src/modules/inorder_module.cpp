#include "modules/inorder_module.h"
#include "modules/payload_module.h"
#include "modules/reliable_module.h"


namespace net_blocks {

inorder_module inorder_module::instance;

void inorder_module::init_module(void) {
	conn_layout.register_member<builder::dyn_var<unsigned int>>("last_sent_sequence");
	conn_layout.register_member<builder::dyn_var<unsigned int>>("last_recv_sequence");
	net_packet.add_member("sequence_number", new generic_integer_member<unsigned int>((int)member_flags::aligned), 4);

	if (inorder_strategy == hold_forever) {
		conn_layout.register_member<builder::dyn_var<void*[INORDER_BUFFER_SIZE]>>("inorder_buffer");
		conn_layout.register_member<builder::dyn_var<unsigned int>>("max_ooo_sequence");
	}
	m_establish_depends = {&payload_module::instance};
	m_destablish_depends = {&payload_module::instance};
	m_send_depends = {&payload_module::instance};
	m_ingress_depends = {&reliable_module::instance};
	framework::instance.register_module(this);
}


module::hook_status inorder_module::hook_establish(builder::dyn_var<connection_t*> c, 
	builder::dyn_var<char*> remote_host, builder::dyn_var<unsigned int> remote_app, 
	builder::dyn_var<unsigned int> local_app) {
	
	// Fairly randomly chosen value :)	
	// TODO: fix this later when we have RNG at runtime
	conn_layout.get(c, "last_sent_sequence") = 1010;
	// 0 is a special value that allows any packet to be accepted
	conn_layout.get(c, "last_recv_sequence") = 0;	

	if (inorder_strategy == hold_forever) {
		for (builder::dyn_var<int> i = 0; i < INORDER_BUFFER_SIZE; i = i + 1) {
			conn_layout.get(c, "inorder_buffer")[i] = 0;
		}
		conn_layout.get(c, "max_ooo_sequence") = 0;
	}
	return module::hook_status::HOOK_CONTINUE;
}

module::hook_status inorder_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p, 
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {

	builder::dyn_var<unsigned int> sq = conn_layout.get(c, "last_sent_sequence") + 1;
	conn_layout.get(c, "last_sent_sequence") = sq;
	net_packet["sequence_number"]->set_integer(p, sq);
		
	return module::hook_status::HOOK_CONTINUE;

}

module::hook_status inorder_module::hook_ingress(packet_t p) {
	// At this point the identifier module has run, so it is safe to look up the 
	// flow identifier from the non-send headers
	builder::dyn_var<connection_t*> c = runtime::to_void_ptr(net_packet["flow_identifier"]->get_integer(p));
	// Implement a simple inorder delivery based on dropping
	builder::dyn_var<unsigned int> last_seq = conn_layout.get(c, "last_recv_sequence");
	builder::dyn_var<unsigned int> packet_seq = net_packet["sequence_number"]->get_integer(p);
	
	if (inorder_strategy == drop_out_of_order) {
		if (last_seq < packet_seq) {
			// All good, we are in order
			// Update the last sequence
			conn_layout.get(c, "last_recv_sequence") = packet_seq;
			// Deliver the packet to the user
			builder::dyn_var<int> payload_len = net_packet["total_len"]->get_integer(p) - ((net_packet.get_total_size()) - 1 - get_headroom());
			runtime::insert_data_queue(conn_layout.get(c, "input_queue"), net_packet["payload"]->get_addr(p), payload_len);	
			return module::hook_status::HOOK_CONTINUE;
		}
		return module::hook_status::HOOK_DROP;
	} else if (inorder_strategy == hold_forever) {
		if (last_seq == 0 || packet_seq == last_seq + 1) {
			// All good, we are in order
			builder::dyn_var<int> payload_len = net_packet["total_len"]->get_integer(p) - ((net_packet.get_total_size()) - 1 - get_headroom());
			runtime::insert_data_queue(conn_layout.get(c, "input_queue"), net_packet["payload"]->get_addr(p), payload_len);	
			conn_layout.get(c, "last_recv_sequence") = packet_seq;
			// Now that we have received this packet, maybe we have more out of order packets to deliver
			for (builder::dyn_var<unsigned int> i = packet_seq + 1; i <= conn_layout.get(c, "max_ooo_sequence"); i = i + 1) {
				builder::dyn_var<unsigned int> index = i % INORDER_BUFFER_SIZE;
				if (conn_layout.get(c, "inorder_buffer")[index] != 0) {
					p = conn_layout.get(c, "inorder_buffer")[index];
					builder::dyn_var<int> payload_len = net_packet["total_len"]->get_integer(p) - ((net_packet.get_total_size()) - 1 - get_headroom());
					runtime::insert_data_queue(conn_layout.get(c, "input_queue"), net_packet["payload"]->get_addr(p), payload_len);	
					conn_layout.get(c, "last_recv_sequence") = i;	
					conn_layout.get(c, "inorder_buffer")[index] = 0;
				} else 
					break;	
			}
		} else {
			// We received a packet out of order, shelve it
			builder::dyn_var<unsigned int> index = packet_seq % INORDER_BUFFER_SIZE;
			conn_layout.get(c, "inorder_buffer")[index] = p;	
			if (conn_layout.get(c, "max_ooo_sequence") < packet_seq) {
				conn_layout.get(c, "max_ooo_sequence") = packet_seq;
			}
		}
	} else {
		builder::dyn_var<int> payload_len = net_packet["total_len"]->get_integer(p) - ((net_packet.get_total_size()) - 1 - get_headroom());
		runtime::insert_data_queue(conn_layout.get(c, "input_queue"), net_packet["payload"]->get_addr(p), payload_len);	
		return module::hook_status::HOOK_CONTINUE;
	}
	return module::hook_status::HOOK_DROP;
}

}
