# Latency Benchmark and Traffic Visualization

This document describes the files that still need to be modified. The
benchmark and Python visualization are **not implemented yet**.

## 1. Add benchmark support

Create:

```text
include/benchmark_support.hpp
tests/test_benchmark_support.cpp
```

The support header should provide:

- TSC calibration against `std::chrono::steady_clock`.
- Safe TSC-to-nanoseconds conversion.
- Percentile calculation for latency samples.
- Counters for attempted, completed, dropped, warmup, and ring-full packets.
- Linux CPU-affinity support with explicit error reporting.
- Windows behavior that reports affinity as unsupported.

Add `benchmark_support_test` to `CMakeLists.txt`.

## 2. Add the paced latency benchmark

Create:

```text
tests/bench_latency.cpp
```

The benchmark should:

- Model four independent ports.
- Generate 64-byte frames.
- Support offered loads of 10%, 50%, 90%, and 100%.
- Stamp every packet with its intended send time before enqueue.
- Count ring-full attempts instead of hiding backpressure.
- Measure:

```text
intended send time -> scheduler dequeue
```

- Calibrate the timestamp counter at startup.
- Include a warmup period.
- Collect at least 1,000,000 completed samples for a full run.
- Report p50, p99, p99.9, and maximum latency.
- Report attempted, completed, dropped, and ring-full counts.
- Report 64-byte serialization time separately.
- Pin workers on Linux when possible.
- Clearly report that Windows runs are unpinned.

Suggested command-line interface:

```text
bench_latency
bench_latency --smoke --allow-unpinned
bench_latency --loads 10,50,90,100 --samples 1000000
bench_latency --output latency_results.jsonl
```

The full benchmark should not be registered as a normal CTest test because
timing-sensitive tests are not deterministic. A short smoke mode may be used
by the verification script.

## 3. Add machine-readable output

Each load should write one JSON Lines record, for example:

```json
{
  "load_percent": 90,
  "samples": 1000000,
  "attempted": 1000000,
  "completed": 1000000,
  "dropped": 0,
  "ring_full": 0,
  "warmup_completed": 100000,
  "calibrated_ghz": 3.42,
  "serialization_ns": 67.2,
  "p50_ns": 1200,
  "p99_ns": 1800,
  "p999_ns": 2400,
  "max_ns": 9000
}
```

Update:

```text
tests/run_verify.sh
```

The script should build the benchmark, run smoke mode, and write the result
file into the build directory.

## 4. Add the Python visualization

Create:

```text
tools/visualize_benchmark.py
tools/test_visualize_benchmark.py
tools/requirements.txt
```

The Python program should:

- Read and validate the JSON Lines output.
- Reject missing required fields with a clear error.
- Sort results by offered load.
- Write `summary.csv`.
- Write latency percentile charts.
- Write throughput/loss charts.
- Animate four port lanes with moving packet markers.
- Display load, completed packets, drops, and ring-full events.

Suggested command:

```bash
python3 tools/visualize_benchmark.py \
  --input build/latency_results.jsonl \
  --output-dir build/latency_report \
  --fps 20
```

Expected output:

```text
build/latency_report/traffic_animation.gif
build/latency_report/latency_percentiles.png
build/latency_report/throughput_and_loss.png
build/latency_report/summary.csv
```

Recommended Python dependencies:

```text
matplotlib
pytest
```

If Pillow is unavailable, the tool should still generate the static charts
and CSV, and print the installation command needed for GIF output.

## 5. Update documentation

Modify:

```text
README.md
```

Document:

- How to build the benchmark.
- How to run smoke mode.
- How to run the full load sweep.
- Why intended-send timestamps are used.
- The latency start and stop points.
- Why Windows results are unpinned.
- How to install Python dependencies.
- How to generate the animation and charts.

## 6. Verification checklist

After implementation:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/bench_latency --smoke --allow-unpinned \
  --output build/latency_results.jsonl
python3 tools/visualize_benchmark.py \
  --input build/latency_results.jsonl \
  --output-dir build/latency_report
```

Verify that:

- Existing functional tests still pass.
- The benchmark produces valid JSON Lines.
- Ring-full and drop counts are visible.
- Four ports appear in the animation.
- All expected charts and CSV files are generated.
- Full benchmark results do not claim pinned Windows performance.
