#ifndef NET_BLOCKS_MODULE_H
#define NET_BLOCKS_MODULE_H
#include "core/connection.h"
#include "core/packet.h"

namespace net_blocks {

class module {

public: 
	enum class hook_status {
		HOOK_CONTINUE,
		HOOK_DROP
	};

	virtual void init_module(void) = 0;
	
	// Various path hooking routines
	virtual hook_status hook_establish(builder::dyn_var<connection_t*> c, 
		builder::dyn_var<char*> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa) {
		return hook_status::HOOK_CONTINUE;	
	}
	
	virtual hook_status hook_destablish(builder::dyn_var<connection_t*> c) {
		return hook_status::HOOK_CONTINUE;	
	}
	

	virtual hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {
		return hook_status::HOOK_CONTINUE;	
	}
	

	virtual hook_status hook_ingress(packet_t) {
		return hook_status::HOOK_CONTINUE;	
	}

	// This hook doesn't require a return value because all modules 
	// need to be initialized
	virtual void hook_net_init(void) {
		return;		
	}	

	// Dependency stuff
	std::vector<module*> m_establish_depends;
	std::vector<module*> m_destablish_depends;
	std::vector<module*> m_send_depends;
	std::vector<module*> m_ingress_depends;
	bool mark_scheduled;
		
	// Enable virtual inheritance
	virtual ~module();
	int m_sequence;

	virtual const char* get_module_name(void) { return "GenericUnamedModule"; }	
};

}
#endif
