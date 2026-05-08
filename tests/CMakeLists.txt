add_executable(protocol_regression
    protocol_regression.c
)

target_link_libraries(protocol_regression PRIVATE artnet)

target_include_directories(protocol_regression PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/artnet
)

add_test(NAME protocol_regression COMMAND protocol_regression)

if(WIN32)
    set_tests_properties(protocol_regression PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:artnet>;$ENV{PATH}"
    )
endif()
