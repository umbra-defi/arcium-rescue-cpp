/**
 * Benchmark Comparison Script
 * 
 * Compares performance between C++ and JavaScript Rescue cipher implementations.
 */

const fs = require('fs');
const path = require('path');

// ANSI color codes for terminal output
const colors = {
    reset: '\x1b[0m',
    bright: '\x1b[1m',
    dim: '\x1b[2m',
    red: '\x1b[31m',
    green: '\x1b[32m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    magenta: '\x1b[35m',
    cyan: '\x1b[36m',
};

/**
 * Format time in appropriate units
 */
function formatTime(ns) {
    if (ns >= 1e9) {
        return `${(ns / 1e9).toFixed(3)} s`;
    } else if (ns >= 1e6) {
        return `${(ns / 1e6).toFixed(3)} ms`;
    } else if (ns >= 1e3) {
        return `${(ns / 1e3).toFixed(3)} μs`;
    }
    return `${ns.toFixed(0)} ns`;
}

/**
 * Calculate speedup ratio
 */
function speedup(cppTime, jsTime) {
    return jsTime / cppTime;
}

/**
 * Format speedup with color
 */
function formatSpeedup(ratio) {
    if (ratio > 1) {
        return `${colors.green}${ratio.toFixed(2)}x faster${colors.reset}`;
    } else if (ratio < 1) {
        return `${colors.red}${(1/ratio).toFixed(2)}x slower${colors.reset}`;
    }
    return `${colors.yellow}same${colors.reset}`;
}

/**
 * Main comparison function
 */
function compareBenchmarks() {
    // Check for result files
    const cppResultsPath = path.join(__dirname, '..', 'build', 'benchmarks', 'benchmark_results_cpp.json');
    const jsResultsPath = path.join(__dirname, 'benchmark_results_js.json');
    
    // Alternative paths
    const altCppPath = path.join(__dirname, 'benchmark_results_cpp.json');
    
    let cppPath = fs.existsSync(cppResultsPath) ? cppResultsPath : altCppPath;
    
    if (!fs.existsSync(cppPath)) {
        console.error(`${colors.red}Error: C++ benchmark results not found.${colors.reset}`);
        console.error(`Expected at: ${cppResultsPath} or ${altCppPath}`);
        console.error('Run the C++ benchmark first: ./build/benchmarks/bench_rescue');
        process.exit(1);
    }
    
    if (!fs.existsSync(jsResultsPath)) {
        console.error(`${colors.red}Error: JavaScript benchmark results not found.${colors.reset}`);
        console.error(`Expected at: ${jsResultsPath}`);
        console.error('Run the JavaScript benchmark first: node interop-tests/bench_rescue.js');
        process.exit(1);
    }
    
    // Load results
    const cppResults = JSON.parse(fs.readFileSync(cppPath, 'utf8'));
    const jsResults = JSON.parse(fs.readFileSync(jsResultsPath, 'utf8'));
    
    console.log('='.repeat(100));
    console.log(`${colors.bright}${colors.cyan}Rescue Cipher Benchmark Comparison: C++ vs JavaScript${colors.reset}`);
    console.log('='.repeat(100));
    console.log();
    console.log(`${colors.dim}C++ Results: ${cppPath}${colors.reset}`);
    console.log(`${colors.dim}JS Results:  ${jsResultsPath}${colors.reset}`);
    console.log(`${colors.dim}C++ Timestamp: ${cppResults.timestamp || 'N/A'}${colors.reset}`);
    console.log(`${colors.dim}JS Timestamp:  ${jsResults.timestamp || 'N/A'}${colors.reset}`);
    console.log(`${colors.dim}Node.js: ${jsResults.node_version || 'N/A'}${colors.reset}`);
    console.log();
    
    // Find matching benchmarks
    const cppBenchmarks = cppResults.benchmarks || {};
    const jsBenchmarks = jsResults.benchmarks || {};
    
    // Common benchmark mappings (C++ name -> JS name)
    const benchmarkMappings = {
        'RescueCipher_Construction': 'RescueCipher_Construction',
        'RescueCipher_Encrypt_1Block': 'RescueCipher_Encrypt_1Block',
        'RescueCipher_Encrypt_10Blocks': 'RescueCipher_Encrypt_10Blocks',
        'RescueCipher_Decrypt_1Block': 'RescueCipher_Decrypt_1Block',
    };
    
    // Print comparison table
    console.log('-'.repeat(100));
    console.log(
        `${'Benchmark'.padEnd(40)}` +
        `${'C++ Time'.padStart(15)}` +
        `${'JS Time'.padStart(15)}` +
        `${'Speedup (C++)'.padStart(25)}`
    );
    console.log('-'.repeat(100));
    
    const comparisons = [];
    
    for (const [cppName, jsName] of Object.entries(benchmarkMappings)) {
        const cppBench = cppBenchmarks[cppName];
        const jsBench = jsBenchmarks[jsName];
        
        if (cppBench && jsBench) {
            const cppTime = cppBench.mean_ns;
            const jsTime = jsBench.mean_ns;
            const ratio = speedup(cppTime, jsTime);
            
            comparisons.push({
                name: cppName,
                cppTime,
                jsTime,
                ratio
            });
            
            console.log(
                `${cppName.padEnd(40)}` +
                `${formatTime(cppTime).padStart(15)}` +
                `${formatTime(jsTime).padStart(15)}` +
                `${formatSpeedup(ratio).padStart(35)}`
            );
        }
    }
    
    // Throughput comparison
    console.log();
    console.log('-'.repeat(100));
    console.log(`${colors.bright}Throughput Comparison${colors.reset}`);
    console.log('-'.repeat(100));
    console.log(
        `${'Size (elements)'.padEnd(20)}` +
        `${'C++ Time'.padStart(15)}` +
        `${'JS Time'.padStart(15)}` +
        `${'Speedup (C++)'.padStart(25)}` +
        `${'C++ MB/s'.padStart(12)}` +
        `${'JS MB/s'.padStart(12)}`
    );
    console.log('-'.repeat(100));
    
    // Find throughput benchmarks
    const throughputSizes = [1, 4, 16, 64, 128, 256, 512, 1024];
    
    for (const size of throughputSizes) {
        const cppKey = `RescueCipher_Throughput/${size}`;
        const jsKey = `RescueCipher_Throughput/${size}`;
        
        const cppBench = cppBenchmarks[cppKey];
        const jsBench = jsBenchmarks[jsKey];
        
        if (cppBench && jsBench) {
            const cppTime = cppBench.mean_ns;
            const jsTime = jsBench.mean_ns;
            const ratio = speedup(cppTime, jsTime);
            
            const cppMBs = cppBench.megabytes_per_second || ((size * 32 * 1e9 / cppTime) / (1024 * 1024));
            const jsMBs = jsBench.megabytes_per_second || ((size * 32 * 1e9 / jsTime) / (1024 * 1024));
            
            comparisons.push({
                name: `Throughput/${size}`,
                cppTime,
                jsTime,
                ratio
            });
            
            console.log(
                `${String(size).padEnd(20)}` +
                `${formatTime(cppTime).padStart(15)}` +
                `${formatTime(jsTime).padStart(15)}` +
                `${formatSpeedup(ratio).padStart(35)}` +
                `${cppMBs.toFixed(2).padStart(12)}` +
                `${jsMBs.toFixed(2).padStart(12)}`
            );
        }
    }
    
    // Summary statistics
    console.log();
    console.log('='.repeat(100));
    console.log(`${colors.bright}Summary${colors.reset}`);
    console.log('='.repeat(100));
    
    if (comparisons.length > 0) {
        const avgSpeedup = comparisons.reduce((sum, c) => sum + c.ratio, 0) / comparisons.length;
        const maxSpeedup = Math.max(...comparisons.map(c => c.ratio));
        const minSpeedup = Math.min(...comparisons.map(c => c.ratio));
        
        console.log(`Benchmarks compared: ${comparisons.length}`);
        console.log(`Average C++ speedup: ${formatSpeedup(avgSpeedup)}`);
        console.log(`Best C++ speedup:    ${formatSpeedup(maxSpeedup)}`);
        console.log(`Worst C++ speedup:   ${formatSpeedup(minSpeedup)}`);
        
        // Show which is generally faster
        const cppFaster = comparisons.filter(c => c.ratio > 1).length;
        const jsFaster = comparisons.filter(c => c.ratio < 1).length;
        const same = comparisons.filter(c => c.ratio === 1).length;
        
        console.log();
        console.log(`C++ faster in ${cppFaster}/${comparisons.length} benchmarks`);
        console.log(`JS faster in ${jsFaster}/${comparisons.length} benchmarks`);
    }
    
    // Save comparison results
    const comparisonOutput = {
        timestamp: new Date().toISOString(),
        cpp_results_path: cppPath,
        js_results_path: jsResultsPath,
        comparisons: comparisons.map(c => ({
            name: c.name,
            cpp_time_ns: c.cppTime,
            js_time_ns: c.jsTime,
            cpp_speedup: c.ratio
        })),
        summary: comparisons.length > 0 ? {
            benchmarks_compared: comparisons.length,
            average_cpp_speedup: comparisons.reduce((sum, c) => sum + c.ratio, 0) / comparisons.length,
            max_cpp_speedup: Math.max(...comparisons.map(c => c.ratio)),
            min_cpp_speedup: Math.min(...comparisons.map(c => c.ratio))
        } : {}
    };
    
    fs.writeFileSync('benchmark_comparison.json', JSON.stringify(comparisonOutput, null, 2));
    console.log();
    console.log(`${colors.dim}Comparison results saved to benchmark_comparison.json${colors.reset}`);
}

// Run comparison
compareBenchmarks();
