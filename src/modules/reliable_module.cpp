#include "modules/reliable_module.h"
#include "builder/static_var.h"
#include "blocks/rce.h"
#include "modules/inorder_module.h"
#include "modules/identifier_module.h"

namespace net_blocks {

reliable_module reliable_module::instance;

void reliable_module::init_module(void) {	
	// This buffer helps us find waiting packets when ACKs arrive
	conn_layout.register_member<builder::dyn_var<void*[REDELIVERY_BUFFER_SIZE]>>("redelivery_buffer");	
	conn_layout.register_member<builder::dyn_var<unsigned int>>("first_unacked_seq");	
	net_packet.add_member("ack_sequence_number", new generic_integer_member<unsigned int>((int)member_flags::aligned), 4);
	net_packet.add_member("redelivery_timer", new generic_integer_member<unsigned long long>((int)member_flags::aligned), 0);
	m_establish_depends = {&inorder_module::instance};
	m_destablish_depends = {&inorder_module::instance};
	m_send_depends = {&inorder_module::instance};
	m_ingress_depends = {&identifier_module::instance};
	framework::instance.register_module(this);
}

module::hook_status reliable_module::hook_establish(builder::dyn_var<connection_t*> c, 
	builder::dyn_var<char*> remote_host, builder::dyn_var<unsigned int> remote_app, 
	builder::dyn_var<unsigned int> local_app) {
	
	for (builder::dyn_var<int> i = 0; i < REDELIVERY_BUFFER_SIZE; i = i + 1) {
		conn_layout.get(c, "redelivery_buffer")[i] = 0;
	}
	return module::hook_status::HOOK_CONTINUE;
}


static builder::dyn_var<void(timer_t*, void*, unsigned long long)> redelivery_timer_callback("nb__reliable_redelivery_timer_cb");


static void redelivery_cb(builder::dyn_var<runtime::timer_t*> t, builder::dyn_var<void*> param, 
		builder::dyn_var<unsigned long long> to) {
	packet_t p = param;	
	builder::dyn_var<int> size = net_packet["total_len"]->get_integer(p);
	runtime::send_packet(p + get_headroom(), size);
	
	runtime::insert_timer(t, to + REDELIVERY_TIMEOUT_MS, redelivery_timer_callback, p);
}

void reliable_module::gen_timer_callback(std::ostream &oss) {	
	auto ast = builder::builder_context().extract_function_ast(redelivery_cb, "nb__reliable_redelivery_timer_cb");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, oss);
}




module::hook_status reliable_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p,
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {
	// We are about to send a new packet, put it in the redelivery buffer
	builder::dyn_var<unsigned int> index = net_packet["sequence_number"]->get_integer(p) % REDELIVERY_BUFFER_SIZE;

	conn_layout.get(c, "redelivery_buffer")[index] = p;
	builder::dyn_var<runtime::timer_t*> t = runtime::alloc_timer();
	runtime::insert_timer(t, runtime::get_time_ms_now() + REDELIVERY_TIMEOUT_MS, redelivery_timer_callback, p);		

	net_packet["redelivery_timer"]->set_integer(p, runtime::to_ull(t));
	net_packet["ack_sequence_number"]->set_integer(p, 0);
	return module::hook_status::HOOK_CONTINUE;
}



module::hook_status reliable_module::hook_ingress(packet_t p) {
	builder::dyn_var<unsigned int> ack_seq = net_packet["ack_sequence_number"]->get_integer(p);	
	builder::dyn_var<connection_t*> c = runtime::to_void_ptr(net_packet["flow_identifier"]->get_integer(p));
	if (ack_seq != 0) {
		builder::dyn_var<unsigned int> index = ack_seq % REDELIVERY_BUFFER_SIZE;
		packet_t p_rem = conn_layout.get(c, "redelivery_buffer")[index];
		builder::dyn_var<runtime::timer_t*> t = runtime::to_void_ptr(net_packet["redelivery_timer"]->get_integer(p_rem));
		runtime::remove_timer(t);
		runtime::return_timer(t);		
		conn_layout.get(c, "redelivery_buffer")[index] = 0;		
		return module::hook_status::HOOK_DROP;
	}	

	builder::dyn_var<unsigned int> seq = net_packet["sequence_number"]->get_integer(p);
	// This is a normal packet, send an ACK	
	packet_t p_ack = runtime::request_send_buffer();
	// Invoke the send path from after this module	
	builder::dyn_var<char[1]> buff = {0};
	builder::dyn_var<int> ret_len;
	builder::dyn_var<int*> ret_len_ptr = &ret_len;
	net_packet["ack_sequence_number"]->set_integer(p_ack, seq);
	// Ack packets have size = 1
	net_packet["total_len"]->set_integer(p_ack, net_packet.get_total_size() - get_headroom());
	for (builder::static_var<unsigned int> i = m_sequence + 1; i < framework::instance.m_registered_modules.size(); i++) {
		builder::static_var<int> s = (int)framework::instance.m_registered_modules[i]->hook_send(c, p_ack, buff, 1, ret_len_ptr);
		if (s == (int)module::hook_status::HOOK_DROP)
			break;
	}
	return module::hook_status::HOOK_CONTINUE;	
}


}
