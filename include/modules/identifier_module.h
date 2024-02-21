#ifndef NET_BLOCKS_IDENTIFIER_MODULE_H
#define NET_BLOCKS_IDENTIFIER_MODULE_H

#include "core/framework.h"

namespace net_blocks {

#define HOST_IDENTIFIER_LEN (6)
#define WILDCARD_APP_ID (0)


extern const char data_queue_t_name[];
using data_queue_t = builder::name<data_queue_t_name>;
extern const char accept_queue_t_name[];
using accept_queue_t = builder::name<accept_queue_t_name>;

class identifier_module: public module {
private:

	unsigned long long host_range_min = 0;
	unsigned long long host_range_max = 0;

	unsigned short app_range_min = 0;
	unsigned short app_range_max = 0;

public:
	
	enum class flow_identifier_t {
		dst_identifiers_only,
		src_dst_identifier,	
	};

	void configFlowIdentifier(flow_identifier_t f) {
		flow_identifier = f;
	}

	void init_module(void);
	
	// Various path hooking routines
	module::hook_status hook_establish(builder::dyn_var<connection_t*> c, 
		builder::dyn_var<unsigned int> h, builder::dyn_var<unsigned int> a, builder::dyn_var<unsigned int> sa);
	
	module::hook_status hook_destablish(builder::dyn_var<connection_t*> c);	

	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len);

	module::hook_status hook_ingress(packet_t);

	void hook_net_init(void);

	
private:
	// Internal functions
	void add_connection(builder::dyn_var<connection_t*> c, builder::dyn_var<unsigned int> appid, 
		builder::dyn_var<unsigned int> src_app_id, builder::dyn_var<unsigned long long> src_host_id);
	bool match_connection(builder::dyn_var<int> &index, 
		builder::dyn_var<unsigned int> dst_app_id, builder::dyn_var<unsigned int> src_app_id, 
		builder::dyn_var<unsigned long long> src_host_id);	
	builder::dyn_var<connection_t*> retrieve_connection(builder::dyn_var<unsigned int> appid,
		builder::dyn_var<unsigned int> src_app_id, builder::dyn_var<unsigned long long> src_host_id);
	void delete_connection(builder::dyn_var<unsigned int> appid,
		builder::dyn_var<unsigned int> src_app_id, builder::dyn_var<unsigned long long> src_host_id);

	builder::dyn_var<connection_t*> retrieve_wildcard_connection(builder::dyn_var<unsigned int> appid);
	flow_identifier_t flow_identifier = flow_identifier_t::dst_identifiers_only;

private:
	identifier_module() = default;
public:
	void set_host_range(unsigned long long min, unsigned long long max) {
		host_range_min = min;
		host_range_max = max;	
	}
	void set_app_range(unsigned short min, unsigned short max) {
		app_range_min = min;
		app_range_max = max;
	}
public:
	static identifier_module instance;

	const char* get_module_name(void) override { return "IdentifierModule"; }
};

}


#endif
