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
    add_executable(packet_generator_test
        tests/packet_generator.cpp
    )
    target_link_libraries(packet_generator_test PRIVATE switch_headers)
    add_test(NAME packet_generator_test COMMAND packet_generator_test)
endif()