#!/bin/bash
#
# Chunked Benchmark Orchestrator
#
# Runs 100k tests in chunks of 1000:
#   1. JS generates 1000 test vectors and benchmarks
#   2. C++ loads those 1000 vectors and benchmarks
#   3. Compare results
#   4. Delete temp files
#   5. Repeat 100 times
#

set -e

# Configuration
CHUNK_SIZE=1000
TOTAL_TESTS=100000
NUM_CHUNKS=$((TOTAL_TESTS / CHUNK_SIZE))

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INTEROP_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$INTEROP_DIR"

# Temp files (in data directory)
CHUNK_FILE="data/chunk_data.json"
JS_STATS="data/chunk_data.json.stats.json"
CPP_STATS="data/chunk_cpp_stats.json"

# Accumulator files
RESULTS_FILE="data/chunked_benchmark_results.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

timestamp() {
    date +"%H:%M:%S.%3N"
}

log() {
    echo -e "[$(timestamp)] $1"
}

log_header() {
    echo ""
    echo -e "${BLUE}════════════════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}════════════════════════════════════════════════════════════════════════════════${NC}"
}

log_section() {
    echo -e "${YELLOW}────────────────────────────────────────────────────────────────────────────────${NC}"
    echo -e "${YELLOW}  $1${NC}"
    echo -e "${YELLOW}────────────────────────────────────────────────────────────────────────────────${NC}"
}

# Check dependencies
check_dependencies() {
    log_header "Checking Dependencies"
    
    if ! command -v node &> /dev/null; then
        log "${RED}ERROR: Node.js not found${NC}"
        exit 1
    fi
    log "Node.js: $(node --version)"
    
    if [ ! -d "node_modules" ]; then
        log "Installing npm dependencies..."
        if command -v pnpm &> /dev/null; then
            pnpm install
        else
            npm install
        fi
    fi
    log "${GREEN}✓ JS dependencies OK${NC}"
    
    if [ ! -f "$INTEROP_DIR/build/chunk_benchmark_cpp" ]; then
        log "Building C++ benchmark..."
        mkdir -p build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j4 chunk_benchmark_cpp
        cd ..
    fi
    log "${GREEN}✓ C++ binary OK${NC}"
}

# Initialize results
init_results() {
    cat > "$RESULTS_FILE" << EOF
{
  "description": "Chunked Benchmark Results (100k tests)",
  "chunk_size": $CHUNK_SIZE,
  "total_tests": $TOTAL_TESTS,
  "num_chunks": $NUM_CHUNKS,
  "start_time": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
  "js_totals": {
    "encrypt_time_ns": 0,
    "decrypt_time_ns": 0,
    "elements": 0
  },
  "cpp_totals": {
    "encrypt_time_ns": 0,
    "decrypt_time_ns": 0,
    "elements": 0
  },
  "chunks": []
}
EOF
}

# Run a single chunk
run_chunk() {
    local chunk_idx=$1
    local total_chunks=$2
    
    log_section "Chunk $((chunk_idx + 1))/$total_chunks (Tests $((chunk_idx * CHUNK_SIZE))-$((chunk_idx * CHUNK_SIZE + CHUNK_SIZE - 1)))"
    
    # Clean up any previous chunk files
    rm -f "$CHUNK_FILE" "$JS_STATS" "$CPP_STATS"
    
    # Run JavaScript
    log ""
    log "${YELLOW}>>> Running JavaScript...${NC}"
    if ! node js/chunk_benchmark_js.js "$chunk_idx" "$CHUNK_SIZE" "$CHUNK_FILE"; then
        log "${RED}ERROR: JavaScript benchmark failed!${NC}"
        exit 1
    fi
    
    # Verify JS output file exists
    if [ ! -f "$CHUNK_FILE" ]; then
        log "${RED}ERROR: JS did not create output file!${NC}"
        exit 1
    fi
    
    # Run C++
    log ""
    log "${YELLOW}>>> Running C++...${NC}"
    if ! ./build/chunk_benchmark_cpp "$CHUNK_FILE" "$CPP_STATS"; then
        log "${RED}ERROR: C++ benchmark failed or mismatch detected!${NC}"
        log "${RED}Stopping immediately as requested.${NC}"
        exit 1
    fi
    
    # Read stats
    local js_enc_ns=$(node -pe "JSON.parse(require('fs').readFileSync('$JS_STATS')).encrypt_time_ns")
    local js_dec_ns=$(node -pe "JSON.parse(require('fs').readFileSync('$JS_STATS')).decrypt_time_ns")
    local js_enc_us=$(node -pe "JSON.parse(require('fs').readFileSync('$JS_STATS')).avg_encrypt_us.toFixed(2)")
    local js_dec_us=$(node -pe "JSON.parse(require('fs').readFileSync('$JS_STATS')).avg_decrypt_us.toFixed(2)")
    local js_enc_tput=$(node -pe "JSON.parse(require('fs').readFileSync('$JS_STATS')).encrypt_throughput.toFixed(0)")
    local js_dec_tput=$(node -pe "JSON.parse(require('fs').readFileSync('$JS_STATS')).decrypt_throughput.toFixed(0)")
    local js_elements=$(node -pe "JSON.parse(require('fs').readFileSync('$JS_STATS')).elements")
    
    local cpp_enc_ns=$(node -pe "JSON.parse(require('fs').readFileSync('$CPP_STATS')).encrypt_time_ns")
    local cpp_dec_ns=$(node -pe "JSON.parse(require('fs').readFileSync('$CPP_STATS')).decrypt_time_ns")
    local cpp_enc_us=$(node -pe "JSON.parse(require('fs').readFileSync('$CPP_STATS')).avg_encrypt_us.toFixed(2)")
    local cpp_dec_us=$(node -pe "JSON.parse(require('fs').readFileSync('$CPP_STATS')).avg_decrypt_us.toFixed(2)")
    local cpp_enc_tput=$(node -pe "JSON.parse(require('fs').readFileSync('$CPP_STATS')).encrypt_throughput.toFixed(0)")
    local cpp_dec_tput=$(node -pe "JSON.parse(require('fs').readFileSync('$CPP_STATS')).decrypt_throughput.toFixed(0)")
    local cpp_elements=$(node -pe "JSON.parse(require('fs').readFileSync('$CPP_STATS')).elements")
    
    # Calculate speedup
    local enc_speedup=$(node -pe "($js_enc_us / $cpp_enc_us).toFixed(2)")
    local dec_speedup=$(node -pe "($js_dec_us / $cpp_dec_us).toFixed(2)")
    
    # Print comparison
    log ""
    log "${GREEN}═══ Chunk $((chunk_idx + 1)) Comparison ═══${NC}"
    log ""
    printf "%-20s %15s %15s %12s\n" "Metric" "JavaScript" "C++" "Speedup"
    printf "%-20s %15s %15s %12s\n" "────────────────────" "───────────────" "───────────────" "────────────"
    printf "%-20s %12s μs %12s μs %10sx\n" "Avg Encrypt" "$js_enc_us" "$cpp_enc_us" "$enc_speedup"
    printf "%-20s %12s μs %12s μs %10sx\n" "Avg Decrypt" "$js_dec_us" "$cpp_dec_us" "$dec_speedup"
    printf "%-20s %12s/s %12s/s\n" "Encrypt Throughput" "$js_enc_tput" "$cpp_enc_tput"
    printf "%-20s %12s/s %12s/s\n" "Decrypt Throughput" "$js_dec_tput" "$cpp_dec_tput"
    log ""
    log "${GREEN}✓ All $CHUNK_SIZE tests passed - JS and C++ outputs match${NC}"
    
    # Update totals in results file (using node for JSON manipulation)
    node -e "
        const fs = require('fs');
        const results = JSON.parse(fs.readFileSync('$RESULTS_FILE'));
        const jsStats = JSON.parse(fs.readFileSync('$JS_STATS'));
        const cppStats = JSON.parse(fs.readFileSync('$CPP_STATS'));
        
        // Update totals (use numbers, not BigInt)
        results.js_totals.encrypt_time_ns = Number(results.js_totals.encrypt_time_ns) + Number(jsStats.encrypt_time_ns);
        results.js_totals.decrypt_time_ns = Number(results.js_totals.decrypt_time_ns) + Number(jsStats.decrypt_time_ns);
        results.js_totals.elements += jsStats.elements;
        
        results.cpp_totals.encrypt_time_ns = Number(results.cpp_totals.encrypt_time_ns) + Number(cppStats.encrypt_time_ns);
        results.cpp_totals.decrypt_time_ns = Number(results.cpp_totals.decrypt_time_ns) + Number(cppStats.decrypt_time_ns);
        results.cpp_totals.elements += cppStats.elements;
        
        // Add chunk summary
        results.chunks.push({
            index: $chunk_idx,
            tests: $CHUNK_SIZE,
            elements: jsStats.elements,
            js: {
                avg_encrypt_us: jsStats.avg_encrypt_us,
                avg_decrypt_us: jsStats.avg_decrypt_us,
                encrypt_throughput: jsStats.encrypt_throughput,
                decrypt_throughput: jsStats.decrypt_throughput
            },
            cpp: {
                avg_encrypt_us: cppStats.avg_encrypt_us,
                avg_decrypt_us: cppStats.avg_decrypt_us,
                encrypt_throughput: cppStats.encrypt_throughput,
                decrypt_throughput: cppStats.decrypt_throughput
            },
            speedup: {
                encrypt: parseFloat('$enc_speedup'),
                decrypt: parseFloat('$dec_speedup')
            }
        });
        
        fs.writeFileSync('$RESULTS_FILE', JSON.stringify(results, null, 2));
    "
    
    # Clean up chunk files
    rm -f "$CHUNK_FILE" "$JS_STATS" "$CPP_STATS"
}

# Print final summary
print_summary() {
    log_header "FINAL SUMMARY"
    
    node -e "
        const fs = require('fs');
        const results = JSON.parse(fs.readFileSync('$RESULTS_FILE'));
        
        const jsEncNs = BigInt(results.js_totals.encrypt_time_ns);
        const jsDecNs = BigInt(results.js_totals.decrypt_time_ns);
        const cppEncNs = results.cpp_totals.encrypt_time_ns;
        const cppDecNs = results.cpp_totals.decrypt_time_ns;
        
        const jsEncUs = Number(jsEncNs) / $TOTAL_TESTS / 1000;
        const jsDecUs = Number(jsDecNs) / $TOTAL_TESTS / 1000;
        const cppEncUs = cppEncNs / $TOTAL_TESTS / 1000;
        const cppDecUs = cppDecNs / $TOTAL_TESTS / 1000;
        
        const jsEncTput = results.js_totals.elements / (Number(jsEncNs) / 1e9);
        const jsDecTput = results.js_totals.elements / (Number(jsDecNs) / 1e9);
        const cppEncTput = results.cpp_totals.elements / (cppEncNs / 1e9);
        const cppDecTput = results.cpp_totals.elements / (cppDecNs / 1e9);
        
        console.log('');
        console.log('Total Tests:    ' + $TOTAL_TESTS.toLocaleString());
        console.log('Total Elements: ' + results.js_totals.elements.toLocaleString());
        console.log('Chunks:         ' + results.chunks.length);
        console.log('');
        console.log('═══════════════════════════════════════════════════════════════════════════════');
        console.log('                              OVERALL RESULTS');
        console.log('═══════════════════════════════════════════════════════════════════════════════');
        console.log('');
        console.log('Metric                    JavaScript              C++           Speedup');
        console.log('─────────────────────────────────────────────────────────────────────────────');
        console.log(\`Total Encrypt Time   \${(Number(jsEncNs)/1e9).toFixed(3).padStart(12)}s   \${(cppEncNs/1e9).toFixed(3).padStart(12)}s   \${(Number(jsEncNs)/cppEncNs).toFixed(2).padStart(8)}x\`);
        console.log(\`Total Decrypt Time   \${(Number(jsDecNs)/1e9).toFixed(3).padStart(12)}s   \${(cppDecNs/1e9).toFixed(3).padStart(12)}s   \${(Number(jsDecNs)/cppDecNs).toFixed(2).padStart(8)}x\`);
        console.log('─────────────────────────────────────────────────────────────────────────────');
        console.log(\`Avg Encrypt/Test     \${jsEncUs.toFixed(2).padStart(12)}μs  \${cppEncUs.toFixed(2).padStart(12)}μs  \${(jsEncUs/cppEncUs).toFixed(2).padStart(8)}x\`);
        console.log(\`Avg Decrypt/Test     \${jsDecUs.toFixed(2).padStart(12)}μs  \${cppDecUs.toFixed(2).padStart(12)}μs  \${(jsDecUs/cppDecUs).toFixed(2).padStart(8)}x\`);
        console.log('─────────────────────────────────────────────────────────────────────────────');
        console.log(\`Encrypt Throughput   \${jsEncTput.toFixed(0).padStart(12)}/s  \${cppEncTput.toFixed(0).padStart(12)}/s  \${(cppEncTput/jsEncTput).toFixed(2).padStart(8)}x\`);
        console.log(\`Decrypt Throughput   \${jsDecTput.toFixed(0).padStart(12)}/s  \${cppDecTput.toFixed(0).padStart(12)}/s  \${(cppDecTput/jsDecTput).toFixed(2).padStart(8)}x\`);
        console.log('═══════════════════════════════════════════════════════════════════════════════');
        console.log('');
        console.log('Interoperability: ALL ' + $TOTAL_TESTS.toLocaleString() + ' TESTS PASSED ✓');
        console.log('');
    "
}

# Main execution
main() {
    log_header "RESCUE CIPHER - Chunked Benchmark (100k Tests)"
    log "Configuration:"
    log "  Total tests: $(printf "%'d" $TOTAL_TESTS)"
    log "  Chunk size:  $(printf "%'d" $CHUNK_SIZE)"
    log "  Num chunks:  $NUM_CHUNKS"
    log ""
    
    check_dependencies
    init_results
    
    log_header "Starting Benchmark"
    
    START_TIME=$(date +%s)
    
    for ((i=0; i<NUM_CHUNKS; i++)); do
        run_chunk $i $NUM_CHUNKS
    done
    
    END_TIME=$(date +%s)
    ELAPSED=$((END_TIME - START_TIME))
    
    print_summary
    
    log "Total execution time: ${ELAPSED}s"
    log "Results saved to: $RESULTS_FILE"
}

# Run main
main "$@"
