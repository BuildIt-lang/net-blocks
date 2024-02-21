#include "core/impls.h"

using namespace net_blocks;


int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cerr << "Usage: " << argv[0] << ": <gen header file> <gen src file>" << std::endl;
		return -1;	
	}	
	interface_module::instance.init_module();


	int scenario = 3;

	identifier_module::instance.configFlowIdentifier(identifier_module::flow_identifier_t::src_dst_identifier);
	routing_module::instance.configEnableRouting();

	signaling_module::instance.configEnableSignaling();
	//signaling_module::instance.configDisableSignaling();

	if (scenario == 1) {
		inorder_module::instance.configInorderStrategy(inorder_module::hold_forever);
		reliable_module::instance.configEnableReliability();	
		checksum_module::instance.configEnableChecksum();
		checksum_module::instance.configChecksumType(checksum_module::checksum_type_t::full_packet);
	} else if (scenario == 2) {
		inorder_module::instance.configInorderStrategy(inorder_module::drop_out_of_order);
		reliable_module::instance.configDisableReliability();	
		checksum_module::instance.configEnableChecksum();
		checksum_module::instance.configChecksumType(checksum_module::checksum_type_t::header_only);
	} else if (scenario == 3) {
		inorder_module::instance.configInorderStrategy(inorder_module::no_inorder);
		reliable_module::instance.configDisableReliability();	
		checksum_module::instance.configDisableChecksum();
	}

	reliable_module::instance.configEnableReliability();	
	inorder_module::instance.configInorderStrategy(inorder_module::hold_forever);

	// Compatibility related schedules
	//payload_module::instance.configEnableCompatibilityMode();
	//identifier_module::instance.configEnableCompatibilityMode();
	//routing_module::instance.configEnableCompatibilityMode();

	// Enable/disable schedules
	//inorder_module::instance.configInorderStrategy(inorder_module::hold_forever);
	//inorder_module::instance.configInorderStrategy(inorder_module::no_inorder);

	



	payload_module::instance.init_module();
	signaling_module::instance.init_module();	
	inorder_module::instance.init_module();
	reliable_module::instance.init_module();
	identifier_module::instance.init_module();	
	signaling_module_after::instance.init_module();	
	routing_module::instance.init_module();
	checksum_module::instance.init_module();
	network_module::instance.init_module();
	
	net_packet.fix_layout();
	net_packet.print_layout(std::cerr);
	
	run_nb_pipeline(argv[1], argv[2]);		

	return 0;
}
