add_executable(protocol_regression
    protocol_regression.c
    network_stub.c
    ${PROJECT_SOURCE_DIR}/artnet/artnet.c
    ${PROJECT_SOURCE_DIR}/artnet/misc.c
    ${PROJECT_SOURCE_DIR}/artnet/receive.c
    ${PROJECT_SOURCE_DIR}/artnet/time_util.c
    ${PROJECT_SOURCE_DIR}/artnet/transmit.c
    ${PROJECT_SOURCE_DIR}/artnet/tod.c
)

set_target_properties(protocol_regression PROPERTIES
    VS_GLOBAL_VcpkgApplocalDeps "false"
)

target_compile_definitions(protocol_regression PRIVATE HAVE_CONFIG_H)

if(WIN32)
    target_compile_definitions(protocol_regression PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
    target_link_libraries(protocol_regression PRIVATE ws2_32)
endif()

target_include_directories(protocol_regression PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/artnet
    ${PROJECT_BINARY_DIR}
)

add_test(NAME protocol_regression COMMAND protocol_regression)

if(WIN32)
    set_tests_properties(protocol_regression PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:artnet>;$ENV{PATH}"
    )
endif()
