add_executable(run_tests
    pdisk_tests.cpp
    vdisk_tests.cpp
    group_tests.cpp
)

target_include_directories(run_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/arctic
)

target_link_libraries(run_tests
    PRIVATE
    gtest_main
    arctic_engine
    model
    controller
    utils
)

include(GoogleTest)
gtest_discover_tests(run_tests)
