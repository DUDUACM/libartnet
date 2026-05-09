if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

set(EXAMPLE_GROUP_MINIMAL
    dmx_tx
    dmx_rx
    timecode_tx
    timecode_rx
    timesync_tx
    diag_monitor
    target_node
)

set(EXAMPLE_GROUP_FOCUSED
    node_manager
    rdm_controller
    file_transfer
    directory_query
)

set(EXAMPLE_GROUP_FULL
    full_node
    full_controller
)

function(add_example_target dir_name)
    add_subdirectory(${dir_name})
    if(MSVC)
        set_target_properties(${dir_name} PROPERTIES
            VS_GLOBAL_VcpkgApplocalDeps "false"
        )
    endif()
endfunction()

if(BUILD_EXAMPLES_MINIMAL)
    add_example_target(dmx_tx)
    add_example_target(dmx_rx)
    add_example_target(timecode_tx)
    add_example_target(timecode_rx)
    add_example_target(timesync_tx)
    add_example_target(diag_monitor)
    add_example_target(target_node)
endif()

if(BUILD_EXAMPLES_FOCUSED)
    add_example_target(node_manager)
    add_example_target(rdm_controller)
    add_example_target(file_transfer)
    add_example_target(directory_query)
endif()

if(BUILD_EXAMPLES_FULL)
    add_example_target(full_node)
    add_example_target(full_controller)
endif()
