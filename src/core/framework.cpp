#include "core/framework.h"
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "core/runtime.h"

namespace net_blocks {

framework framework::instance;

void framework::register_module(module *m) {
	m_registered_modules.push_back(m);
}
// Implementations for various paths	
builder::dyn_var<connection_t*> framework::run_establish_path(builder::dyn_var<char*> host_id, builder::dyn_var<unsigned int> app_id, 
	builder::dyn_var<unsigned int> src_app_id) {
	// Establish a new connection object
	builder::dyn_var<connection_t*> conn = runtime::malloc(runtime::connection_t_size());

	for (builder::static_var<unsigned int> i = 0; i < m_registered_modules.size(); i++) {
		builder::static_var<int> s = (int)m_registered_modules[i]->hook_establish(conn, host_id, app_id, src_app_id);
		if (s == (int)module::hook_status::HOOK_DROP)
			break;
	}

	return conn;	
}

void framework::run_destablish_path(builder::dyn_var<connection_t*> c) {
	for (builder::static_var<unsigned int> i = 0; i < m_registered_modules.size(); i++) {
		builder::static_var<int> s = (int)m_registered_modules[i]->hook_destablish(c);
		if (s == (int)module::hook_status::HOOK_DROP)
			break;
	}
	runtime::free(c);
}
builder::dyn_var<int> framework::run_send_path(builder::dyn_var<connection_t*> conn, 
	builder::dyn_var<char*> buff, builder::dyn_var<int> len) {
	
	packet_t p = runtime::malloc(MTU_SIZE);
	builder::dyn_var<int> ret_len = 0;
	builder::dyn_var<int*> ret_len_ptr = &ret_len;
			
	for (builder::static_var<unsigned int> i = 0; i < m_registered_modules.size(); i++) {
		builder::static_var<int> s = (int)m_registered_modules[i]->hook_send(conn, p, buff, len, ret_len_ptr);
		if (s == (int)module::hook_status::HOOK_DROP)
			break;
	}
	
	return ret_len;
}
void framework::run_ingress_path(packet_t p) {
	for (builder::static_var<unsigned int> i = 0; i < m_registered_modules.size(); i++) {
		builder::static_var<int> s = (int)m_registered_modules[i]->hook_ingress(p);
		if (s == (int)module::hook_status::HOOK_DROP)
			break;
	}	
}
// Definition for the main net_packet dynamic layout
dynamic_layout net_packet;

}
