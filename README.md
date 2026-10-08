# switch_for_x86
software model of switch
```bash
PS C:\Users\shree\go\switch_packet> cmake -S . -B build -DBUILD_TESTING=ON
>> cmake --build build --config Release
>> ctest --test-dir build -C Release --output-on-failure
CMake Error: The source directory "C:/Users/shree/go/switch_packet" does notappear to contain CMakeLists.txt.
Specify --help for usage, or press the help button on the CMake GUI.
Error: C:/Users/shree/go/switch_packet/build is not a directory
Failed to change working directory to "C:/Users/shree/go/switch_packet/build": No such file or directory
PS C:\Users\shree\go\switch_packet> cmake -S . -B build -DBUILD_TESTING=ON
>> cmake --build build --config Release
>> ctest --test-dir build -C Release --output-on-failure^C
PS C:\Users\shree\go\switch_packet> cd C:\Users\shree\go\switch_packet\switch_for_x86
>> 
>> cmake -S . -B build -DBUILD_TESTING=ON
>> cmake --build build --config Release
>> ctest --test-dir build -C Release --output-on-failure
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
-- Configuring done (0.1s)
-- Generating done (1.7s)
-- Build files have been written to: C:/Users/shree/go/switch_packet/switch_for_x86/build
MSBuild version 17.14.23+b0019275e for .NET Framework

  1>Checking Build System
  Building Custom Rule C:/Users/shree/go/switch_packet/switch_for_x86/CMakeL
  ists.txt
  bench_lpm.cpp
  bench_lpm.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\build\
  Release\bench_lpm.exe
  Building Custom Rule C:/Users/shree/go/switch_packet/switch_for_x86/CMakeL
  ists.txt
  test_lpm_reference.cpp
  lpm_reference_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x
  86\build\Release\lpm_reference_test.exe
  Building Custom Rule C:/Users/shree/go/switch_packet/switch_for_x86/CMakeL
  ists.txt
  packet_generator.cpp
  packet_generator_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_fo
  r_x86\build\Release\packet_generator_test.exe
  Building Custom Rule C:/Users/shree/go/switch_packet/switch_for_x86/CMakeL
  ists.txt
  test_parser.cpp
  parser_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\buil
  d\Release\parser_test.exe
  Building Custom Rule C:/Users/shree/go/switch_packet/switch_for_x86/CMakeL
  ists.txt
  test_pool_mt.cpp
  pool_mt_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\bui
  ld\Release\pool_mt_test.exe
  Building Custom Rule C:/Users/shree/go/switch_packet/switch_for_x86/CMakeL
  ists.txt
  test_ring_stress.cpp
  ring_stress_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86
  \build\Release\ring_stress_test.exe
  Building Custom Rule C:/Users/shree/go/switch_packet/switch_for_x86/CMakeL
  ists.txt
  test_scheduler.cpp
  scheduler_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\b
  uild\Release\scheduler_test.exe
  Building Custom Rule C:/Users/shree/go/switch_packet/switch_for_x86/CMakeL
  ists.txt
  main.cpp
  switch_for_x86.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\b
  uild\Release\switch_for_x86.exe
  Building Custom Rule C:/Users/shree/go/switch_packet/switch_for_x86/CMakeL
  ists.txt
Test project C:/Users/shree/go/switch_packet/switch_for_x86/build
    Start 1: packet_generator_test
1/6 Test #1: packet_generator_test ............   Passed    0.16 sec
    Start 2: parser_test
2/6 Test #2: parser_test ......................   Passed    0.85 sec
    Start 3: lpm_reference_test
3/6 Test #3: lpm_reference_test ...............   Passed    0.24 sec
    Start 4: scheduler_test
4/6 Test #4: scheduler_test ...................   Passed    0.92 sec
    Start 5: ring_stress_test
5/6 Test #5: ring_stress_test .................   Passed    0.11 sec
    Start 6: pool_mt_test
6/6 Test #6: pool_mt_test .....................   Passed    1.49 sec

100% tests passed, 0 tests failed out of 6

Total Test time (real) =   6.51 sec
PS C:\Users\shree\go\switch_packet\switch_for_x86> .\build\Release\bench_lpm.exe 5000 1
threads=1 37.9 ns/lookup 114 cycles @3.0GHz nodes=27022
PS C:\Users\shree\go\switch_packet\switch_for_x86> .\build\Release\parser_test.exe
>> .\build\Release\lpm_reference_test.exe
>> .\build\Release\scheduler_test.exe
>> .\build\Release\ring_stress_test.exe 10000
>> .\build\Release\pool_mt_test.exe
>> .\build\Release\packet_generator_test.exe
RESULT: PASSED
RESULT: PASSED
RESULT: PASSED
RESULT: PASSED
RESULT: PASSED
```



## Running from WSL

From WSL, run the script from the project directory:

```bash
cd /mnt/c/Users/shree/go/switch_packet/switch_for_x86
bash tests/run_verify.sh
```

The script converts `/mnt/c/...` paths automatically when WSL finds the
Windows `cmake.exe`.

For a native Linux build instead, install the Linux tools first:

```bash
sudo apt update
sudo apt install cmake g++
bash tests/run_verify.sh
```
runing verify
```bash
shree@MSI:/mnt/c/Users/shree/go/switch_packet/switch_for_x86$ cd /mnt/c/Users/shree/go/switch_packet/switch_for_x86
bash tests/run_verify.sh
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
-- Configuring done (0.1s)
-- Generating done (0.8s)
-- Build files have been written to: C:/Users/shree/go/switch_packet/switch_for_x86/build
MSBuild version 17.14.23+b0019275e for .NET Framework

  bench_lpm.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\build\Release\bench_lpm.exe
  lpm_reference_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\build\Release\lpm_reference_test.exe
  packet_generator_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\build\Release\packet_generator_test.exe
  parser_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\build\Release\parser_test.exe
  pool_mt_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\build\Release\pool_mt_test.exe
  ring_stress_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\build\Release\ring_stress_test.exe
  scheduler_test.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\build\Release\scheduler_test.exe
  switch_for_x86.vcxproj -> C:\Users\shree\go\switch_packet\switch_for_x86\build\Release\switch_for_x86.exe
Test project C:/Users/shree/go/switch_packet/switch_for_x86/build
    Start 1: packet_generator_test
1/6 Test #1: packet_generator_test ............   Passed    0.11 sec
    Start 2: parser_test
2/6 Test #2: parser_test ......................   Passed    0.34 sec
    Start 3: lpm_reference_test
3/6 Test #3: lpm_reference_test ...............   Passed    0.17 sec
    Start 4: scheduler_test
4/6 Test #4: scheduler_test ...................   Passed    0.04 sec
    Start 5: ring_stress_test
5/6 Test #5: ring_stress_test .................   Passed    0.04 sec
    Start 6: pool_mt_test
6/6 Test #6: pool_mt_test .....................   Passed    0.05 sec

100% tests passed, 0 tests failed out of 6

Total Test time (real) =   0.83 sec
=== bench_lpm
threads=1 50.5 ns/lookup 152 cycles @3.0GHz nodes=27022
```
