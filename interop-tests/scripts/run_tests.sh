#!/bin/bash
# Rescue Cipher Interoperability Test Runner

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INTEROP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$INTEROP_DIR"

echo "========================================"
echo "Rescue Cipher Interoperability Tests"
echo "========================================"
echo

# Step 1: Generate fresh JS test vectors
echo "Step 1: Generating JavaScript test vectors..."
node js/generate_test_vectors.js
echo

# Step 2: Build C++ test
echo "Step 2: Building C++ interop test..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cd ..
echo

# Step 3: Run C++ test
echo "Step 3: Running C++ interop test..."
./build/test_interop data/test_vectors_js.json data/test_vectors_cpp.json
echo

# Step 4: Compare results
echo "Step 4: Comparing results..."
node js/compare_results.js

echo
echo "========================================"
echo "Interoperability tests complete!"
echo "========================================"
