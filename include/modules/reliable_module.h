#ifndef NET_BLOCKS_RELIABLE_MODULE_H
#define NET_BLOCKS_RELIABLE_MODULE_H

#include "core/framework.h"

#define REDELIVERY_BUFFER_SIZE (32)
#define REDELIVERY_TIMEOUT_MS (5)

namespace net_blocks {

class reliable_module: public module {
public:
	void init_module(void);

	module::hook_status hook_establish(builder::dyn_var<connection_t*> c, 
		builder::dyn_var<char*> remote_host, builder::dyn_var<unsigned int> remote_app, 
		builder::dyn_var<unsigned int> local_app);
	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len);

	module::hook_status hook_ingress(packet_t);

	void gen_timer_callback(std::ostream &oss);


private:		
	reliable_module() = default;
public:
	static reliable_module instance;
};

}



#endif
