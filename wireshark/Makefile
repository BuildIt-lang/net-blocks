WIRESHARK_SRC ?= ../wireshark
WIRESHARK_BUILD := $(BASE_DIR)/wireshark-build
WIRESHARK_PLUGIN := $(BASE_DIR)/wireshark

.PHONY: wireshark-plugin

wireshark-plugin: $(WIRESHARK_PLUGIN)/packet-net_blocks-generated.c
	cmake \
		-S $(WIRESHARK_SRC) \
		-B $(WIRESHARK_BUILD) \
		-DCUSTOM_PLUGIN_SRC_DIR=$(abspath $(WIRESHARK_PLUGIN))

	cmake --build $(WIRESHARK_BUILD) --target plugins
