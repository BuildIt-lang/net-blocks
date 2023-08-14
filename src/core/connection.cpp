#include "core/connection.h"


namespace net_blocks {

const char connection_t_name[] = CONNECTION_T_STR;
dynamic_object conn_layout;
dynamic_object init_params;
dynamic_object net_state;

int dynamic_object::dynamic_object_counter = 0;
void dynamic_object::generate_struct_decl(std::ostream &oss, std::string struct_name) {
	oss << "struct dynamic_struct_" << dynamic_object_counter << ";" << std::endl;
	oss << "typedef struct dynamic_struct_" << dynamic_object_counter << " " << struct_name << ";" << std::endl;	
	oss << "struct dynamic_struct_" << dynamic_object_counter << " {" << std::endl;
	for (auto v: m_decl_field) {
		printer::indent(oss, 1);
		block::c_code_generator::generate_code(v.second, oss, 0);
	}
	oss << "};" << std::endl; 
	dynamic_object_counter++;
}

builder::builder dynamic_object::get(builder::dyn_var<connection_t*> c, std::string name) {
	// TODO: assert that member is valid and registered
	// This functionality of producing the appropriate expression can be offloaded
	// to the map
	return (builder::builder)(builder::dyn_var<int>)builder::as_member(((builder::dyn_var<connection_t>)(builder::cast)(c[0])).addr(), name);
}

}
