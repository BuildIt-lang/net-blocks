#ifndef NET_BLOCKS_PAYLOAD_MODULE_H
#define NET_BLOCKS_PAYLOAD_MODULE_H

#include "core/framework.h"

namespace net_blocks {

class payload_module: public module {
private:
	unsigned int max_packet_len = 0;
public:
	void init_module(void);
	
	// Various path hooking routines	
	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len);

	module::hook_status hook_ingress(packet_t);


private:
	payload_module() = default;
public:
	void set_max_length(unsigned int max) {
		max_packet_len = max;
	}
public:
	static payload_module instance;
	const char* get_module_name(void) override { return "PayloadModule"; }

};


}

#endif
