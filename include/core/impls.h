#ifndef NETBLOCKS_CORE_IMPLS_H
#define NETBLOCKS_CORE_IMPLS_H

#include "modules/interface_module.h"
#include "modules/identifier_module.h"
#include "modules/inorder_module.h"
#include "modules/reliable_module.h"
#include "modules/payload_module.h"
#include "modules/network_module.h"
#include "modules/checksum_module.h"
#include "modules/signaling_module.h"
#include "modules/routing_module.h"
#include "builder/builder_context.h"
#include "blocks/c_code_generator.h"
#include "blocks/rce.h"
namespace net_blocks {


void run_nb_pipeline(std::string header_file, std::string output_file);

}

#endif


