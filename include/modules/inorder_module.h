#ifndef NET_BLOCKS_INORDER_MODULE_H
#define NET_BLOCKS_INORDER_MODULE_H

#include "core/framework.h"

#define INORDER_BUFFER_SIZE (32)

namespace net_blocks {

class inorder_module: public module {
public:
	
	enum inorder_strategy_t {
		drop_out_of_order,
		hold_forever,
		no_inorder,	
	};
	
	void configInorderStrategy(inorder_strategy_t t) {
		inorder_strategy = t;
	}
	
	void init_module(void);

	module::hook_status hook_establish(builder::dyn_var<connection_t*> c, 
		builder::dyn_var<char*> remote_host, builder::dyn_var<unsigned int> remote_app, 
		builder::dyn_var<unsigned int> local_app);

	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len);

	module::hook_status hook_ingress(packet_t);

private:
	inorder_strategy_t inorder_strategy = drop_out_of_order;

private:
	inorder_module() = default;
public:
	static inorder_module instance;
};


}

#endif
