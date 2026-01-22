#!/bin/bash
# Run both C++ and JavaScript benchmarks and compare results

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=============================================="
echo "Rescue Cipher Benchmark Suite"
echo "=============================================="
echo

# Check if C++ benchmark exists
CPP_BENCH="$BUILD_DIR/benchmarks/bench_rescue"
if [ ! -f "$CPP_BENCH" ]; then
    echo "C++ benchmark not found. Building..."
    cd "$PROJECT_ROOT"
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make bench_rescue -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    echo
fi

# Run C++ benchmark
echo "=============================================="
echo "Running C++ Benchmark"
echo "=============================================="
cd "$BUILD_DIR/benchmarks"
./bench_rescue --benchmark_counters_tabular=true

# Copy results to interop-tests for comparison
cp -f benchmark_results_cpp.json "$SCRIPT_DIR/" 2>/dev/null || true

echo
echo "=============================================="
echo "Running JavaScript Benchmark"
echo "=============================================="
cd "$SCRIPT_DIR"

# Check if node_modules exists
if [ ! -d "node_modules" ]; then
    echo "Installing dependencies..."
    pnpm install
fi

node bench_rescue.js

echo
echo "=============================================="
echo "Comparing Results"
echo "=============================================="
node compare_benchmarks.js

echo
echo "=============================================="
echo "Benchmark Suite Complete"
echo "=============================================="
