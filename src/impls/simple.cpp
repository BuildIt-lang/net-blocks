#include "modules/interface_module.h"
#include "modules/identifier_module.h"
#include "modules/inorder_module.h"
#include "modules/reliable_module.h"
#include "modules/payload_module.h"
#include "modules/network_module.h"
#include "modules/checksum_module.h"
#include "modules/signaling_module.h"
#include "core/genplugin.h"
#include "builder/builder_context.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
#include <fstream>

using namespace net_blocks;

static void generate_headers(std::ostream &oss) {
	oss << "#include \"nb_runtime.h\"" << std::endl;
}

static builder::dyn_var<connection_t*> establish_wrapper(builder::dyn_var<unsigned long long> h, builder::dyn_var<unsigned int> app, 
	builder::dyn_var<unsigned int> sa, callback_t c) {
	return interface_module::instance.establish_impl(h, app, sa, c);
}
static void generate_establish(std::ostream &oss) {
	auto ast = builder::builder_context().extract_function_ast(establish_wrapper, "nb__establish");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, oss);
}


static void destablish_wrapper(builder::dyn_var<connection_t*> c) {
	interface_module::instance.destablish_impl(c);
}
static void generate_destablish(std::ostream &oss) {
	auto ast = builder::builder_context().extract_function_ast(destablish_wrapper, "nb__destablish");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, oss);
	
}


static builder::dyn_var<int> send_wrapper(builder::dyn_var<connection_t*> c,
	builder::dyn_var<char*> buff, builder::dyn_var<int> len) {
	return interface_module::instance.send_impl(c, buff, len);
}
static void generate_send(std::ostream &oss) {
	auto ast = builder::builder_context().extract_function_ast(send_wrapper, "nb__send");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, oss);	
}

static void run_ingress_step_wrapper(builder::dyn_var<void*> p, builder::dyn_var<int> len) {
	interface_module::instance.run_ingress_step(p, len);
}
static void generate_ingress_step(std::ostream &oss) {
	auto ast = builder::builder_context().extract_function_ast(run_ingress_step_wrapper, "nb__run_ingress_step");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, oss);	
}

static void net_init_wrapper(void) {
	interface_module::instance.net_init_impl();
}
static void generate_net_init(std::ostream &oss) {
	auto ast = builder::builder_context().extract_function_ast(net_init_wrapper, "nb__net_init");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, oss);	
}

static void generate_connection_layout(std::string fname) {
	std::ofstream hoss(fname);
	hoss << "#pragma once" << std::endl;
	hoss << "#include \"nb_data_queue.h\"" << std::endl;
	conn_layout.generate_struct_decl(hoss, "nb__connection_t");
	net_state.generate_struct_decl(hoss, "nb__net_state_t");
	hoss << "static const int nb__packet_headroom = " << get_headroom() << ";" << std::endl;
	hoss.close();
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cerr << "Usage: " << argv[0] << ": <gen header file> <gen source file> [<gen wireshark plugin>]" << std::endl;
		return -1;	
	}	
	interface_module::instance.init_module();

	payload_module::instance.init_module();

	//inorder_module::instance.configInorderStrategy(inorder_module::hold_forever);
	inorder_module::instance.configInorderStrategy(inorder_module::no_inorder);
	inorder_module::instance.init_module();

	signaling_module::instance.init_module();	
	
	//reliable_module::instance.init_module();

	identifier_module::instance.configFlowIdentifier(identifier_module::flow_identifier_t::src_dst_identifier);
	identifier_module::instance.init_module();	

	checksum_module::instance.init_module();

	network_module::instance.init_module();
	
	net_packet.fix_layout();
	net_packet.print_layout(std::cerr);
	
	generate_connection_layout(argv[1]);

	std::ofstream oss(argv[2]);

	generate_headers(oss);

	generate_net_init(oss);
	generate_establish(oss);		
	generate_destablish(oss);
	generate_send(oss);
	generate_ingress_step(oss);
	reliable_module::instance.gen_timer_callback(oss);

	oss.close();

	if (argc >= 4) {
		std::ofstream poss(argv[3]);
		plugin::generate_wireshark_plugin(poss);
		poss.close();
	}


	return 0;
}
