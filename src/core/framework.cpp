#include "core/framework.h"
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "core/runtime.h"

namespace net_blocks {

framework framework::instance;


static bool debug_module_paths = false;


static void debug_establish_drop(module* m) {
	if (debug_module_paths) {
		runtime::info_log("Establish connection dropped", m->get_module_name());
	}
}
static void debug_destablish_drop(module* m) {
	if (debug_module_paths) {
		runtime::info_log("Destablish connection dropped", m->get_module_name());
	}
}
static void debug_send_drop(module* m, packet_t p, builder::dyn_var<int> len) {
	if (debug_module_paths) {
		runtime::info_log("Packet [%p, %d] dropped on send path", m->get_module_name(), p, len);
	}
}
static void debug_ingress_drop(module* m, packet_t p, builder::dyn_var<int> len) {
	if (debug_module_paths) {
		runtime::info_log("Packet [%p, %d] dropped on ingress path", m->get_module_name(), p, len);
	}
}

void framework::register_module(module *m) {
	m_registered_modules.push_back(m);
	m->m_sequence = m_registered_modules.size() - 1;
}
// Implementations for various paths	
void framework::run_establish_path(builder::dyn_var<connection_t*> conn, builder::dyn_var<unsigned int> host_id, builder::dyn_var<unsigned int> app_id, 
	builder::dyn_var<unsigned int> src_app_id) {

	for (builder::static_var<unsigned int> i = 0; i < m_registered_modules.size(); i++) {
		builder::static_var<int> s = (int)m_registered_modules[i]->hook_establish(conn, host_id, app_id, src_app_id);
		if (s == (int)module::hook_status::HOOK_DROP) {			
			debug_establish_drop(m_registered_modules[i]);	
			break;
		}
	}
}

void framework::run_destablish_path(builder::dyn_var<connection_t*> c) {
	for (builder::static_var<unsigned int> i = 0; i < m_registered_modules.size(); i++) {
		builder::static_var<int> s = (int)m_registered_modules[i]->hook_destablish(c);
		if (s == (int)module::hook_status::HOOK_DROP) {
			debug_destablish_drop(m_registered_modules[i]);	
			break;
		}
	}
}
builder::dyn_var<int> framework::run_send_path(builder::dyn_var<connection_t*> conn, 
	builder::dyn_var<char*> buff, builder::dyn_var<int> len) {
	
	packet_t p = runtime::request_send_buffer();
	builder::dyn_var<int> ret_len = 0;
	builder::dyn_var<int*> ret_len_ptr = &ret_len;
			
	for (builder::static_var<unsigned int> i = 0; i < m_registered_modules.size(); i++) {
		builder::static_var<int> s = (int)m_registered_modules[i]->hook_send(conn, p, buff, len, ret_len_ptr);
		if (s == (int)module::hook_status::HOOK_DROP) {
			debug_send_drop(m_registered_modules[i], p, len);	
			break;
		}
	}
	
	return ret_len;
}

void framework::run_ingress_path(packet_t p) {
	for (builder::static_var<int> i = m_registered_modules.size() - 1; i >= 0; i--) {
		builder::static_var<int> s = (int)m_registered_modules[i]->hook_ingress(p);
		if (s == (int)module::hook_status::HOOK_DROP) {
			debug_ingress_drop(m_registered_modules[i], p, 0);	
			break;
		}
	}	
}

void framework::run_net_init_path(void) {
	runtime::net_state_obj = runtime::malloc(runtime::size_of(runtime::net_state_obj[0]));
	runtime::nb_assert(runtime::net_state_obj != 0, "malloc failed while allocating net_state");
	for (builder::static_var<unsigned int> i = 0; i < m_registered_modules.size(); i++) {
		m_registered_modules[i]->hook_net_init();
	}
}


void framework::finalize_paths(void) {
	unsigned int total_modules = m_registered_modules.size();

	// Finalize the establish path
	m_establish_path.clear();	
	for (auto module: m_registered_modules) {
		module->mark_scheduled = false;
	}
	while (m_establish_path.size() < total_modules) {
		for (auto module: m_registered_modules) {
			if (!module->mark_scheduled) {
				bool all_satisfied = true;
				for (auto d: module->m_establish_depends) {
					if (d->mark_scheduled == false) {
						all_satisfied = false;
						break;
					}	
				}
				if (all_satisfied) {
					m_establish_path.push_back(module);	
					module->mark_scheduled = true;
					continue;
				}
			}
		}
		assert(false && "Circular dependency while scheduling\n");
	}
	// Finalize the deestablish path
	m_destablish_path.clear();	
	for (auto module: m_registered_modules) {
		module->mark_scheduled = false;
	}
	while (m_destablish_path.size() < total_modules) {
		for (auto module: m_registered_modules) {
			if (!module->mark_scheduled) {
				bool all_satisfied = true;
				for (auto d: module->m_destablish_depends) {
					if (d->mark_scheduled == false) {
						all_satisfied = false;
						break;
					}	
				}
				if (all_satisfied) {
					m_destablish_path.push_back(module);	
					module->mark_scheduled = true;
					continue;
				}
			}
		}
		assert(false && "Circular dependency while scheduling\n");
	}
	// Finalize the send path
	m_send_path.clear();	
	for (auto module: m_registered_modules) {
		module->mark_scheduled = false;
	}
	while (m_send_path.size() < total_modules) {
		for (auto module: m_registered_modules) {
			if (!module->mark_scheduled) {
				bool all_satisfied = true;
				for (auto d: module->m_send_depends) {
					if (d->mark_scheduled == false) {
						all_satisfied = false;
						break;
					}	
				}
				if (all_satisfied) {
					m_send_path.push_back(module);	
					module->mark_scheduled = true;
					continue;
				}
			}
		}
		assert(false && "Circular dependency while scheduling\n");
	}
	// Finalize the send path
	m_ingress_path.clear();	
	for (auto module: m_registered_modules) {
		module->mark_scheduled = false;
	}
	while (m_ingress_path.size() < total_modules) {
		for (auto module: m_registered_modules) {
			if (!module->mark_scheduled) {
				bool all_satisfied = true;
				for (auto d: module->m_ingress_depends) {
					if (d->mark_scheduled == false) {
						all_satisfied = false;
						break;
					}	
				}
				if (all_satisfied) {
					m_ingress_path.push_back(module);	
					module->mark_scheduled = true;
					continue;
				}
			}
		}
		assert(false && "Circular dependency while scheduling\n");
	}

}

// Definition for the main net_packet dynamic layout
dynamic_layout net_packet;

int get_headroom(void) {
	return net_packet.group_start_offset(1);
}



}
