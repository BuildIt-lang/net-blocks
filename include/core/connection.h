#ifndef NET_BLOCKS_CONNECTION_H
#define NET_BLOCKS_CONNECTION_H
#include <map>
#include "builder/dyn_var.h"
#include "blocks/c_code_generator.h"
#include "core/runtime.h"

namespace net_blocks {

#define CONNECTION_T_STR "nb__connection_t"
extern const char connection_t_name[];

using connection_t = builder::name<connection_t_name>;

struct dynamic_object {
	static int dynamic_object_counter;

	using type = builder::name<connection_t_name>;
	using ptr = builder::dyn_var<type*>;

	std::map<std::string, int> m_allowed_members;
	std::map<std::string, block::decl_stmt::Ptr> m_decl_field;

	template <typename T>
	void register_member(std::string s) {
		m_allowed_members[s] = 1;
		// This is a low level trick 
		// to create a declaration for the members of the struct
		block::decl_stmt::Ptr decl = std::make_shared<block::decl_stmt>();
		block::var::Ptr var = std::make_shared<block::var>();
		var->var_type = T::create_block_type();
		var->var_name = s;
		decl->decl_var = var;
		decl->init_expr = nullptr;
		m_decl_field[s] = decl;	
	}	
	
	void generate_struct_decl(std::ostream &oss, std::string struct_name);

	builder::builder get(builder::dyn_var<connection_t*> c, std::string name);
};

extern dynamic_object conn_layout;
extern dynamic_object init_params;
extern dynamic_object net_state;


// Helper function to access runtime state
static inline builder::builder get_state(std::string name) {
	return net_state.get(runtime::net_state_obj, name);
}


}

#endif
