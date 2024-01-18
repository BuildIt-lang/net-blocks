#include "core/genplugin.h"
#include "blocks/c_code_generator.h"
#include "builder/static_var.h"
#include "blocks/rce.h"

namespace net_blocks {
namespace plugin {

static const char proto_tree_name[] = "proto_tree";
using proto_tree_t = builder::name<proto_tree_name>;

static const char tvb_name[] = "tvbuff_t";
using tvbuff_t = builder::name<tvb_name>;

builder::dyn_var<void(void)> proto_tree_add_subtree_format = builder::as_global("proto_tree_add_subtree_format");

static void iterate_and_generate(packet_t packet, builder::dyn_var<proto_tree_t*> tree, builder::dyn_var<tvbuff_t*> tvb) {

	packet = packet - get_headroom();

	for (builder::static_var<size_t> g = 1; g < net_packet.m_registered_members.size(); g++) {
		for (builder::static_var<size_t> m = 0; m < net_packet.m_registered_members[g].size(); m++) {
			std::string field_name = net_packet.m_registered_members[g][m];	

			builder::dyn_var<char*> val = net_packet.m_members_map[field_name]->serialize_for_debug(packet);
			int start = net_packet.m_members_map[field_name]->get_offset() / byte_size - get_headroom();
			int len = align_to(net_packet.m_members_map[field_name]->get_size(), byte_size) / byte_size;
			//printf("Start = %d\n", start);
			//printf("len = %d\n", len);
			proto_tree_add_subtree_format(tree, tvb, start, len, 0, 0, "%s: %s", net_packet.m_registered_members[g][m], val);
			runtime::free(val);
		}
	}
}


void generate_wireshark_plugin(std::ostream &oss) {
	oss << "#include \"config.h\"\n#include <epan/packet.h>\n#include<stdio.h>" << std::endl;
	auto ast = builder::builder_context().extract_function_ast(iterate_and_generate, "run_plugin");
	block::eliminate_redundant_vars(ast);
	block::c_code_generator::generate_code(ast, oss);			
}


}
}
