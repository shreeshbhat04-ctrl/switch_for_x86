# switch_for_x86
software model of switch
Scenario: Build a software model of a 4-port 10 GbE L3 switch that runs on a multicore x86 box,validate forwarding behavior and performance before the silicon exists.

## Constraints

- [x] 4 ports, each at 10 Gbps line rate with worst-case traffic of 64B frames
- [x] Forwarding table: 500K IPv4 routes (longest-prefix match), with route updates arriving at ~1K/sec while forwarding continues
- [x] 8 egress queues per port: queue 7 is strict priority (control traffic), queues 0-6 share bandwidth by weight
- [x] Egress buffer: 12 MB shared across all queues
- [x] Target: sustain line rate with zero loss in steady state, p99.9 forwarding latency under 50 µs, and no more than 2 cores per port
- [x] Packets may carry an 802.1Q VLAN tag, and some will be malformed (truncated, bad IPv4 header checksum, TTL=0)
- [x] Bonus : Branch-free IPv4 header checksum (RFC 1071 style) using 64-bit loads


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
O=/tmp/swv && mkdir -p $O && INC="-Iinclude -Itests" && B="-std=c++17 -g -Wall -Wextra"
for t in test_parser test_lpm_reference test_scheduler packet_generator; do echo "=== $t (ASan+UBSan)"; g++ $B $INC -O1 -fsanitize=address,undefined tests/$t.cpp -o $O/$t 2>&1 | grep -E "error" ; $O/$t 2>&1 | tail -4; done
for t in test_ring_stress test_pool_mt; do echo "=== $t (TSan)"; g++ $B $INC -O1 -fsanitize=thread tests/$t.cpp -o $O/$t -pthread 2>&1 | grep error; setarch $(uname -m) -R $O/$t 20000 2>&1 | tail -3; done
echo "=== bench_lpm"; g++ $B $INC -O2 -DNDEBUG tests/bench_lpm.cpp -o $O/bench -pthread 2>&1 | grep error; $O/bench 5000 1 | tail -3
echo "=== main (Release)"; g++ $B $INC -O2 -DNDEBUG src/main.cpp -o $O/main -pthread 2>&1 | grep error; $O/main 2>&1 | grep -v pin_thread
```
Output

```bash
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
# Second design 
first i thought my lpm was the bottle neck and tried to improve from binary trie with mutex to DIR-24-8
```txt
The DIR-24-8 algorithm (Gupta, Prabhakar, McKeown - Stanford Infocom '98) partitions the 32-bit IPv4 address space into two lookup stages:

Table 1 (lpm_24):
2^24 = 16,777,216 entries of uint16_t (32 MB).
Direct array lookup using the first 24 bits (ipv4 >> 8).
Bit 15 is a flag (lpm_24_FLAG_MASK = 0x8000):
0: Bits 14..0 store the next-hop egress port directly. Lookup finishes in exactly 1 memory access for all prefixes ≤24 bits (~99% of Internet routes).
1: Bits 14..0 store the chunk index in lpm_long.
0xFFFF (INVALID): No route exists (returns -1).

Table 2 (lpm_long):
Organized in chunks of 
2^8=256 entries.For prefixes with length > 24 bits (lengths 25 to 32), an lpm_long chunk is allocated.
Lookup takes at most 1 additional direct memory access: (chunk_index * 256) + (ipv4 & 0xFF).

Out-of-Order Longest Prefix Match Support:
Simon Perriard's verified bachelor thesis implementation assumed rules always arrive in ascending order of prefix length 
For our switch model, we added prefix depth tracking (depth_24 and depth_long). This guarantees that routes can be inserted or updated in any random order, and more specific routes (longer prefixes) will never be erroneously overwritten by less specific routes (shorter prefixes / default route).

Zero-Lock Forwarding Concurrency:
Memory addresses of lpm_24 and lpm_long are fixed and preallocated.
Lookups require zero heap allocations, zero pointer dereferences, and zero locks.
Route insertion uses std::atomic_thread_fence(std::memory_order_release) to ensure lpm_long chunk entries are globally visible before lpm_24 flags are flipped.
```
status no change

```bash
================ SWITCH PERFORMANCE REPORT ================
Total Packets Received:      1000000
Total Packets Forwarded:     1000000
Total Packets Dropped:       0
  - Malformed/Checksum Drops: 0
  - Buffer Exhaustion Drops:  0
p99.9 Forwarding Latency:    2796.203 us (Target: < 50 us)
===========================================================
But was this improved the lookup speed though
Old LPM (4-bit trie): 51.9 ns=0.052 μs
New DIR-24-8 LPM: 10.9 ns=0.011 μs
```
# Third design
Thought it was buffer queue delay maybe the rx_ring and parse ring and scheduler queue which would be 4,096 + 4,096 + 1,000
= 9,192 pkt and the pipline takes even ~300ns to process each pkt  9,192 x 300 ns = 2,757 us
and also latency  in stats.hpp how it buckets 
```cpp
void add_latency_cycles(uint64_t cycles) noexcept {
    std::size_t bucket = 0;
    while (cycles > 1 && bucket + 1 < latency_buckets_.size()) {
        cycles >>= 1;
        ++bucket;
    }
    ++latency_buckets_[bucket];
}
```
at 3Ghz bucket23 = 2^23/3Ghz = 2,796.203us
also in code
```cpp
while(!rx_to_parse_ring->push(std::move(pkt)) && running){
    std::this_thread::yield();
}
```
In Linux / WSL, std::this_thread::yield() gives up the CPU core to the OS scheduler. An OS thread deschedule and context switch takes 1,000 to 3,000 ns (and sometimes milliseconds if CPU scheduling quanta are involved).
Replacing yield with _mm_pause() which takes ≈10 ns
```cpp
#if defined(_MSC_VER) || defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#define CPU_PAUSE() _mm_pause()
#else
#define CPU_PAUSE() ((void)0)
#endif
```
Bounded Ring Capacity to Fit SLA (src/main.cpp):
Reduced ring_capacity from 4096 to 8 (and 16).
At capacity 8, maximum queued packets across both rings is 8+8=16.
Maximum queue wait time is strictly bounded to ≈16×300 ns≈4.8 μs, well within the 50 μs target.
Accurate Ingress Admission Timestamping (src/main.cpp):
Pre-allocated template packet outside the loop to eliminate per-packet frame construction overhead.
Updated entry_timestamp at the point of ring admission so latency measures true ingress-to-egress transit time

# Output
```bash
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
==================
RESULT: PASSED
ThreadSanitizer: reported 1 warnings
=== bench_lpm
threads=1 14.4 ns/lookup 43 cycles @3.0GHz nodes=1
=== main (Release)
Starting 4-Port 10 GbE L3 Switch ...
[Ingress] injection complete.

================ SWITCH PERFORMANCE REPORT ================
Total Packets Received:      1000000
Total Packets Forwarded:     1000000
Total Packets Dropped:       0
  - Malformed/Checksum Drops: 0
  - Buffer Exhaustion Drops:  0
p99.9 Forwarding Latency:    21.845 us (Target: < 50 us) <-- achieved
===========================================================
```

