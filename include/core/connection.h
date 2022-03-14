#ifndef NET_BLOCKS_CONNECTION_H
#define NET_BLOCKS_CONNECTION_H
#include <map>
#include "builder/dyn_var.h"

namespace net_blocks {

#define CONNECTION_T_STR "nb__connection_t"
extern const char connection_t_name[];

using connection_t = builder::name<connection_t_name>;

struct dynamic_object {
	using type = builder::name<connection_t_name>;
	using ptr = builder::dyn_var<type*>;

	std::map<std::string, int> m_allowed_members;
	// This needs to be extended to take type of the member
	// for automatic struct generation
	void register_member(std::string s) {
		m_allowed_members[s] = 1;
	}	

	builder::builder get(builder::dyn_var<connection_t*> c, std::string name) {
		// TODO: assert that member is valid and registered
		// This functionality of producing the appropriate expression can be offloaded
		// to the map
		return (builder::builder)(builder::dyn_var<int>)builder::as_member_of(((builder::dyn_var<connection_t>)(builder::cast)(c[0])).addr(), name);
	}
};

extern dynamic_object conn_layout;
extern dynamic_object init_params;

}

#endif
