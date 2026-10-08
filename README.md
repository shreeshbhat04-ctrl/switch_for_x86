# switch_for_x86
software model of switch
Scenario: Build a software model of a 4-port 10 GbE L3 switch that runs on a multicore x86 box,validate forwarding behavior and performance before the silicon exists.

## Constraints

- [x] 4 ports, each at 10 Gbps line rate with worst-case traffic of 64B frames
- [x] Forwarding table: 500K IPv4 routes (longest-prefix match), with route updates arriving at ~1K/sec while forwarding continues
- [x] 8 egress queues per port: queue 7 is strict priority (control traffic), queues 0-6 share bandwidth by weight
- [x] Egress buffer: 12 MB shared across all queues
- [ ] Target: sustain line rate with zero loss in steady state, p99.9 forwarding latency under 50 µs, and no more than 2 cores per port
- [x] Packets may carry an 802.1Q VLAN tag, and some will be malformed (truncated, bad IPv4 header checksum, TTL=0)


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
## failure of the first design
```bash
cd /home/sw && O=/tmp/swv && mkdir -p $O && INC="-Iinclude -Itests" && B="-std=c++17 -g -Wall -Wextra"
for t in test_parser test_lpm_reference test_scheduler packet_generator; do echo "=== $t (ASan+UBSan)"; g++ $B $INC -O1 -fsanitize=address,undefined tests/$t.cpp -o $O/$t 2>&1 | grep -E "error" ; $O/$t 2>&1 | tail -4; done
for t in test_ring_stress test_pool_mt; do echo "=== $t (TSan)"; g++ $B $INC -O1 -fsanitize=thread tests/$t.cpp -o $O/$t -pthread 2>&1 | grep error; $O/$t 20000 2>&1 | tail -3; done
echo "=== bench_lpm"; g++ $B $INC -O2 -DNDEBUG tests/bench_lpm.cpp -o $O/bench -pthread 2>&1 | grep error; $O/bench 5000 1 | tail -3
echo "=== main (Release)"; g++ $B $INC -O2 -DNDEBUG src/main.cpp -o $O/main -pthread 2>&1 | grep error; $O/main 2>&1 | grep -v pin_thread
Output

=== test_parser (ASan+UBSan)
RESULT: PASSED
=== test_lpm_reference (ASan+UBSan)
RESULT: PASSED
=== test_scheduler (ASan+UBSan)
RESULT: PASSED
=== packet_generator (ASan+UBSan)
=== test_ring_stress (TSan)
RESULT: PASSED
=== test_pool_mt (TSan)
RESULT: PASSED
=== bench_lpm
threads=1 51.9 ns/lookup 156 cycles @3.0GHz nodes=27022
=== main (Release)
Starting 4-Port 10 GbE L3 Switch ...
[Ingress] injection complete.

================ SWITCH PERFORMANCE REPORT ================
Total Packets Received:      1000000
Total Packets Forwarded:     1000000
Total Packets Dropped:       0
  - Malformed/Checksum Drops: 0
  - Buffer Exhaustion Drops:  0
p99.9 Forwarding Latency:    2796.203 us (Target: < 50 us) <---
===========================================================
```
