#include "modules/interface_module.h"
#include "modules/identifier_module.h"
#include "modules/payload_module.h"
#include "modules/network_module.h"
#include "builder/builder_context.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"

using namespace net_blocks;

static void generate_headers(void) {
	std::cout << "#include \"nb_runtime.h\"" << std::endl;
}

static builder::dyn_var<connection_t*> establish_wrapper(builder::dyn_var<unsigned int> h, builder::dyn_var<unsigned int> app, 
	builder::dyn_var<unsigned int> sa, callback_t c) {
	return interface_module::instance.establish_impl(h, app, sa, c);
}
static void generate_establish(void) {
	auto ast = builder::builder_context().extract_function_ast(establish_wrapper, "nb__establish");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, std::cout);
}


static void destablish_wrapper(builder::dyn_var<connection_t*> c) {
	interface_module::instance.destablish_impl(c);
}
static void generate_destablish(void) {
	auto ast = builder::builder_context().extract_function_ast(destablish_wrapper, "nb__destablish");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, std::cout);
	
}


static builder::dyn_var<int> send_wrapper(builder::dyn_var<connection_t*> c,
	builder::dyn_var<char*> buff, builder::dyn_var<int> len) {
	return interface_module::instance.send_impl(c, buff, len);
}
static void generate_send(void) {
	auto ast = builder::builder_context().extract_function_ast(send_wrapper, "nb__send");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, std::cout);	
}

static void run_ingress_step_wrapper(void) {
	interface_module::instance.run_ingress_step();
}
static void generate_ingress_step(void) {
	auto ast = builder::builder_context().extract_function_ast(run_ingress_step_wrapper, "nb__run_ingress_step");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, std::cout);	
}


int main(int argc, char* argv[]) {
	interface_module::instance.init_module();

	identifier_module m1;
	m1.init_module();	

	payload_module m2;
	m2.init_module();

	network_module m3;
	m3.init_module();

	generate_headers();
	generate_establish();		
	generate_destablish();
	generate_send();
	generate_ingress_step();
	return 0;
}
