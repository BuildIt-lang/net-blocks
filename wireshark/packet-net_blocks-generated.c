#include "config.h"
#include <epan/packet.h>
#include<stdio.h>
void run_plugin (unsigned char* arg0, proto_tree* arg1, tvbuff_t* arg2) {
  unsigned char* var2 = arg0;
  var2 = var2 - 24;
  unsigned long long int var5;
  unsigned long long int* var7 = (void*)(var2 + 24);
  var5 = var7[0] & 281474976710655ll;
  char* var10 = malloc(1024);
  char* var11 = var10;
  var11 = var11 + sprintf(var11, "%llu (", var5);
  unsigned char* var12 = (void*)((&(var5)));
  for (int var13 = 0; var13 < 8; var13 = var13 + 1) {
    var11 = var11 + sprintf(var11, "%02x:", var12[var13]);
  }
  var11 = var11 + sprintf(var11 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 0, 6, 0, 0, "%s: %s", "dst_host_id", var10);
  free(var10);
  unsigned long long int var16;
  unsigned long long int* var18 = (void*)(var2 + 30);
  var16 = var18[0] & 281474976710655ll;
  char* var21 = malloc(1024);
  char* var22 = var21;
  var22 = var22 + sprintf(var22, "%llu (", var16);
  unsigned char* var23 = (void*)((&(var16)));
  for (int var24 = 0; var24 < 8; var24 = var24 + 1) {
    var22 = var22 + sprintf(var22, "%02x:", var23[var24]);
  }
  var22 = var22 + sprintf(var22 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 6, 6, 0, 0, "%s: %s", "src_host_id", var21);
  free(var21);
  unsigned long long int var27;
  unsigned int* var29 = (void*)(var2 + 36);
  var27 = var29[0];
  char* var32 = malloc(1024);
  char* var33 = var32;
  var33 = var33 + sprintf(var33, "%llu (", var27);
  unsigned char* var34 = (void*)((&(var27)));
  for (int var35 = 0; var35 < 4; var35 = var35 + 1) {
    var33 = var33 + sprintf(var33, "%02x:", var34[var35]);
  }
  var33 = var33 + sprintf(var33 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 12, 4, 0, 0, "%s: %s", "dst_global_id", var32);
  free(var32);
  unsigned long long int var38;
  unsigned int* var40 = (void*)(var2 + 40);
  var38 = var40[0];
  char* var43 = malloc(1024);
  char* var44 = var43;
  var44 = var44 + sprintf(var44, "%llu (", var38);
  unsigned char* var45 = (void*)((&(var38)));
  for (int var46 = 0; var46 < 4; var46 = var46 + 1) {
    var44 = var44 + sprintf(var44, "%02x:", var45[var46]);
  }
  var44 = var44 + sprintf(var44 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 16, 4, 0, 0, "%s: %s", "src_global_id", var43);
  free(var43);
  unsigned long long int var49;
  unsigned short int* var51 = (void*)(var2 + 44);
  var49 = var51[0];
  char* var54 = malloc(1024);
  char* var55 = var54;
  var55 = var55 + sprintf(var55, "%llu (", var49);
  unsigned char* var56 = (void*)((&(var49)));
  for (int var57 = 0; var57 < 2; var57 = var57 + 1) {
    var55 = var55 + sprintf(var55, "%02x:", var56[var57]);
  }
  var55 = var55 + sprintf(var55 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 20, 2, 0, 0, "%s: %s", "dst_app_id", var54);
  free(var54);
  unsigned long long int var60;
  unsigned short int* var62 = (void*)(var2 + 46);
  var60 = var62[0];
  char* var65 = malloc(1024);
  char* var66 = var65;
  var66 = var66 + sprintf(var66, "%llu (", var60);
  unsigned char* var67 = (void*)((&(var60)));
  for (int var68 = 0; var68 < 2; var68 = var68 + 1) {
    var66 = var66 + sprintf(var66, "%02x:", var67[var68]);
  }
  var66 = var66 + sprintf(var66 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 22, 2, 0, 0, "%s: %s", "src_app_id", var65);
  free(var65);
  unsigned long long int var71;
  int* var73 = (void*)(var2 + 48);
  var71 = var73[0];
  char* var76 = malloc(1024);
  char* var77 = var76;
  var77 = var77 + sprintf(var77, "%llu (", var71);
  unsigned char* var78 = (void*)((&(var71)));
  for (int var79 = 0; var79 < 4; var79 = var79 + 1) {
    var77 = var77 + sprintf(var77, "%02x:", var78[var79]);
  }
  var77 = var77 + sprintf(var77 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 24, 4, 0, 0, "%s: %s", "total_len", var76);
  free(var76);
  unsigned long long int var82;
  unsigned int* var84 = (void*)(var2 + 52);
  var82 = var84[0];
  char* var87 = malloc(1024);
  char* var88 = var87;
  var88 = var88 + sprintf(var88, "%llu (", var82);
  unsigned char* var89 = (void*)((&(var82)));
  for (int var90 = 0; var90 < 4; var90 = var90 + 1) {
    var88 = var88 + sprintf(var88, "%02x:", var89[var90]);
  }
  var88 = var88 + sprintf(var88 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 28, 4, 0, 0, "%s: %s", "sequence_number", var87);
  free(var87);
  unsigned long long int var93;
  unsigned int* var95 = (void*)(var2 + 56);
  var93 = var95[0];
  char* var98 = malloc(1024);
  char* var99 = var98;
  var99 = var99 + sprintf(var99, "%llu (", var93);
  unsigned char* var100 = (void*)((&(var93)));
  for (int var101 = 0; var101 < 4; var101 = var101 + 1) {
    var99 = var99 + sprintf(var99, "%02x:", var100[var101]);
  }
  var99 = var99 + sprintf(var99 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 32, 4, 0, 0, "%s: %s", "ack_sequence_number", var98);
  free(var98);
  unsigned long long int var104;
  unsigned short int* var106 = (void*)(var2 + 60);
  var104 = var106[0];
  char* var109 = malloc(1024);
  char* var110 = var109;
  var110 = var110 + sprintf(var110, "%llu (", var104);
  unsigned char* var111 = (void*)((&(var104)));
  for (int var112 = 0; var112 < 2; var112 = var112 + 1) {
    var110 = var110 + sprintf(var110, "%02x:", var111[var112]);
  }
  var110 = var110 + sprintf(var110 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 36, 2, 0, 0, "%s: %s", "checksum", var109);
  free(var109);
  unsigned long long int var115;
  char* var117 = (void*)(var2 + 62);
  var115 = var117[0];
  char* var120 = malloc(1024);
  char* var121 = var120;
  var121 = var121 + sprintf(var121, "%llu (", var115);
  unsigned char* var122 = (void*)((&(var115)));
  for (int var123 = 0; var123 < 1; var123 = var123 + 1) {
    var121 = var121 + sprintf(var121, "%02x:", var122[var123]);
  }
  var121 = var121 + sprintf(var121 - 1, ")");
  proto_tree_add_subtree_format(arg1, arg2, 38, 1, 0, 0, "%s: %s", "payload", var120);
  free(var120);
}

