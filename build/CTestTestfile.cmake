# CMake generated Testfile for 
# Source directory: C:/Users/shree/go/switch_packet/switch_for_x86
# Build directory: C:/Users/shree/go/switch_packet/switch_for_x86/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(packet_generator_test "C:/Users/shree/go/switch_packet/switch_for_x86/build/Debug/packet_generator_test.exe")
  set_tests_properties(packet_generator_test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/shree/go/switch_packet/switch_for_x86/CMakeLists.txt;29;add_test;C:/Users/shree/go/switch_packet/switch_for_x86/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(packet_generator_test "C:/Users/shree/go/switch_packet/switch_for_x86/build/Release/packet_generator_test.exe")
  set_tests_properties(packet_generator_test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/shree/go/switch_packet/switch_for_x86/CMakeLists.txt;29;add_test;C:/Users/shree/go/switch_packet/switch_for_x86/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(packet_generator_test "C:/Users/shree/go/switch_packet/switch_for_x86/build/MinSizeRel/packet_generator_test.exe")
  set_tests_properties(packet_generator_test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/shree/go/switch_packet/switch_for_x86/CMakeLists.txt;29;add_test;C:/Users/shree/go/switch_packet/switch_for_x86/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(packet_generator_test "C:/Users/shree/go/switch_packet/switch_for_x86/build/RelWithDebInfo/packet_generator_test.exe")
  set_tests_properties(packet_generator_test PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/shree/go/switch_packet/switch_for_x86/CMakeLists.txt;29;add_test;C:/Users/shree/go/switch_packet/switch_for_x86/CMakeLists.txt;0;")
else()
  add_test(packet_generator_test NOT_AVAILABLE)
endif()
