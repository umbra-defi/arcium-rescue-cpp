#!/bin/bash
# 100k Benchmark and Interop Test Runner
# Runs both JavaScript and C++ implementations and compares results

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INTEROP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$INTEROP_DIR"

echo "========================================"
echo "100k Rescue Cipher Benchmark Suite"
echo "========================================"
echo ""

# Check if pnpm is installed
if ! command -v pnpm &> /dev/null; then
    echo "pnpm not found, using npm instead..."
    PKG_MGR="npm"
else
    PKG_MGR="pnpm"
fi

# Install JS dependencies if needed
if [ ! -d "node_modules" ]; then
    echo "Installing JavaScript dependencies..."
    $PKG_MGR install
fi

# Build C++ benchmark
echo ""
echo "Building C++ benchmark..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4 benchmark_100k
cd ..

# Run JavaScript benchmark
echo ""
echo "========================================"
echo "Running JavaScript benchmark (100k)..."
echo "========================================"
node js/benchmark_100k.js

# Run C++ benchmark
echo ""
echo "========================================"
echo "Running C++ benchmark (100k)..."
echo "========================================"
./build/benchmark_100k data/test_vectors_100k.ndjson

# Compare results
echo ""
echo "========================================"
echo "Performance Comparison"
echo "========================================"

# Read results from JSON files
JS_RESULTS=$(cat data/benchmark_results_100k_js.json)
CPP_RESULTS=$(cat data/benchmark_results_100k_cpp.json)

# Extract metrics using node
node -e "
const js = JSON.parse(\`$JS_RESULTS\`);
const cpp = JSON.parse(\`$CPP_RESULTS\`);

const jsEnc = js.benchmark_results.encrypt_throughput_elements_per_sec;
const jsDec = js.benchmark_results.decrypt_throughput_elements_per_sec;
const cppEnc = cpp.benchmark_results.encrypt_throughput_elements_per_sec;
const cppDec = cpp.benchmark_results.decrypt_throughput_elements_per_sec;

const jsEncUs = js.benchmark_results.avg_encrypt_time_us;
const jsDecUs = js.benchmark_results.avg_decrypt_time_us;
const cppEncUs = cpp.benchmark_results.avg_encrypt_time_us;
const cppDecUs = cpp.benchmark_results.avg_decrypt_time_us;

console.log('');
console.log('Metric                     JavaScript          C++        Speedup');
console.log('─'.repeat(70));
console.log(\`Avg encrypt time      \${jsEncUs.toFixed(3).padStart(12)} μs  \${cppEncUs.toFixed(3).padStart(12)} μs  \${(jsEncUs/cppEncUs).toFixed(2).padStart(7)}x\`);
console.log(\`Avg decrypt time      \${jsDecUs.toFixed(3).padStart(12)} μs  \${cppDecUs.toFixed(3).padStart(12)} μs  \${(jsDecUs/cppDecUs).toFixed(2).padStart(7)}x\`);
console.log('');
console.log(\`Encrypt throughput    \${jsEnc.toFixed(0).padStart(12)} el/s \${cppEnc.toFixed(0).padStart(12)} el/s \${(cppEnc/jsEnc).toFixed(2).padStart(7)}x\`);
console.log(\`Decrypt throughput    \${jsDec.toFixed(0).padStart(12)} el/s \${cppDec.toFixed(0).padStart(12)} el/s \${(cppDec/jsDec).toFixed(2).padStart(7)}x\`);
console.log('');

// Interop results
if (cpp.interop_results) {
    const ir = cpp.interop_results;
    console.log('Interoperability:');
    console.log(\`  Passed: \${ir.passed.toLocaleString()} / \${(ir.passed + ir.failed).toLocaleString()}\`);
    console.log(\`  Success rate: \${ir.success_rate_percent.toFixed(2)}%\`);
}
"

echo ""
echo "========================================"
echo "Benchmark complete!"
echo "========================================"
