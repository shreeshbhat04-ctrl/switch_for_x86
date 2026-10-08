cmake_minimum_required(VERSION 3.16)

project(switch_for_x86 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(switch_headers INTERFACE)
target_include_directories(switch_headers INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_executable(switch_for_x86
    src/main.cpp
)
target_link_libraries(switch_for_x86 PRIVATE switch_headers)

if(UNIX AND NOT APPLE)
    target_link_libraries(switch_for_x86 PRIVATE pthread)
endif()

include(CTest)
if(BUILD_TESTING)
    function(add_switch_test target source)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE switch_headers)
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    add_switch_test(packet_generator_test tests/packet_generator.cpp)
    add_switch_test(parser_test tests/test_parser.cpp)
    add_switch_test(lpm_reference_test tests/test_lpm_reference.cpp)
    add_switch_test(scheduler_test tests/test_scheduler.cpp)
    add_executable(ring_stress_test tests/test_ring_stress.cpp)
    target_link_libraries(ring_stress_test PRIVATE switch_headers)
    add_test(NAME ring_stress_test COMMAND ring_stress_test 10000)
    set_tests_properties(ring_stress_test PROPERTIES
        TIMEOUT 30
        PASS_REGULAR_EXPRESSION "RESULT: PASSED"
    )
    add_switch_test(pool_mt_test tests/test_pool_mt.cpp)

    add_executable(bench_lpm tests/bench_lpm.cpp)
    target_link_libraries(bench_lpm PRIVATE switch_headers)
endif()