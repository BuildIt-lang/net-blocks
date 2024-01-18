#ifndef GEN_PLUGIN_H
#define GEN_PLUGIN_H
#include "core/framework.h"
#include <fstream>

namespace net_blocks {
namespace plugin {

void generate_wireshark_plugin(std::ostream &oss);
}
}


#endif
