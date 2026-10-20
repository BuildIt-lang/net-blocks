include(WiresharkPlugin)

set_module_info(nbxplugin 0 1 0 0)

set(DISSECTOR_SRC
    nbx_protocol.c
    packet-net_blocks-generated.c
)

set(PLUGIN_FILES
    plugin.c
    ${DISSECTOR_SRC}
)

set_source_files_properties(
    ${PLUGIN_FILES}
    PROPERTIES
    COMPILE_FLAGS
    "${WERROR_COMMON_FLAGS}"
)

register_plugin_files(
    plugin.c
    plugin
    ${DISSECTOR_SRC}
)

add_plugin_library(nbxplugin epan)

target_link_libraries(nbxplugin epan)

install_plugin(nbxplugin epan)
