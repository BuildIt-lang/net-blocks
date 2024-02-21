#ifndef NET_BLOCKS_CHECKSUM_MODULE_H
#define NET_BLOCKS_CHECKSUM_MODULE_H

#include "core/framework.h"

namespace net_blocks {

class checksum_module: public module {
public:
	enum checksum_type_t {
		full_packet,
		header_only
	};

private:
	bool is_enabled = true;
	enum checksum_type_t checksum_type = full_packet;

public:
	void init_module(void);

	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len);

	module::hook_status hook_ingress(packet_t);

	void configEnableChecksum(void) {
		is_enabled = true;
	}
	void configDisableChecksum(void) {
		is_enabled = false;
	}

	void configChecksumType(checksum_type_t ct) {
		checksum_type = ct;
	}	
	
private:
	checksum_module() = default;

public:
	static checksum_module instance;

	const char* get_module_name(void) override { return "ChecksumModule"; }

};

}

#endif
