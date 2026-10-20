#include "config.h"
#include <epan/packet.h>

static int proto_nbx = -1;

void run_plugin (const unsigned char* arg0, proto_tree* arg1, tvbuff_t* arg2);

static gboolean dissect_nbx_heur(tvbuff_t* tvb, packet_info *pinfo, proto_tree *tree, void *data _U_) {
        int len = tvb_captured_length(tvb);

        col_set_str(pinfo->cinfo, COL_PROTOCOL, "NBX");
        /* Clear the info column */
        col_clear(pinfo->cinfo,COL_INFO);
        col_set_str(pinfo->cinfo, COL_INFO, "NBX packet");

        proto_item *ti = proto_tree_add_item(tree, proto_nbx, tvb, 0, -1, ENC_NA);
        proto_tree* tit = proto_item_add_subtree(ti, 0);
        run_plugin(tvb_get_ptr(tvb, 0, len), tit, tvb);

        return true;
}

void proto_register_nbx(void) {
        proto_nbx = proto_register_protocol("NBX Protocol", "NBX", "nbx");
}




void proto_reg_handoff_nbx(void) {
        heur_dissector_add("eth", dissect_nbx_heur, "NBX over ethernet", "nbx_eth", proto_nbx, HEURISTIC_ENABLE);

}
