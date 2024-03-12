#include "core/impls.h"

using namespace net_blocks;


int main(int argc, char* argv[]) {
	if (argc < 3) {
		std::cerr << "Usage: " << argv[0] << ": <gen header file> <gen src file>" << std::endl;
		return -1;	
	}	
	interface_module::instance.init_module();

	identifier_module::instance.configFlowIdentifier(identifier_module::flow_identifier_t::src_dst_identifier);
	routing_module::instance.configEnableRouting();
	signaling_module::instance.configEnableSignaling();
	inorder_module::instance.configInorderStrategy(inorder_module::hold_forever);
	reliable_module::instance.configEnableReliability();	
	checksum_module::instance.configEnableChecksum();
	checksum_module::instance.configChecksumType(checksum_module::checksum_type_t::full_packet);

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
