/**
 * JavaScript Benchmark for Rescue Cipher
 * 
 * Measures performance of @arcium-hq/client's RescueCipher implementation
 * for comparison against the C++ implementation.
 */

const crypto = require('crypto');
const fs = require('fs');
const { RescueCipher } = require('@arcium-hq/client');

// Field modulus
const P = 2n ** 255n - 19n;

// Configuration
const WARMUP_ITERATIONS = 10;
const BENCHMARK_ITERATIONS = 100;
const BLOCK_SIZE = 3; // Same as RESCUE_CIPHER_BLOCK_SIZE in C++

/**
 * Generate a random shared secret (32 bytes)
 */
function generateSharedSecret() {
    return crypto.randomBytes(32);
}

/**
 * Generate a random nonce (16 bytes)
 */
function generateNonce() {
    return crypto.randomBytes(16);
}

/**
 * Generate random field element
 */
function randomFieldElement() {
    const bytes = crypto.randomBytes(32);
    let value = 0n;
    for (let j = 0; j < 32; j++) {
        value |= BigInt(bytes[j]) << BigInt(j * 8);
    }
    return value % P;
}

/**
 * Generate random plaintext of given length
 */
function generatePlaintext(length) {
    const plaintext = [];
    for (let i = 0; i < length; i++) {
        plaintext.push(randomFieldElement());
    }
    return plaintext;
}

/**
 * Benchmark utility class
 */
class Benchmark {
    constructor(name) {
        this.name = name;
        this.times = [];
    }

    /**
     * Run a benchmark with warmup and measurement phases
     */
    run(fn, iterations = BENCHMARK_ITERATIONS, warmupIterations = WARMUP_ITERATIONS) {
        // Warmup phase
        for (let i = 0; i < warmupIterations; i++) {
            fn();
        }

        // Force GC if available
        if (global.gc) {
            global.gc();
        }

        // Measurement phase
        this.times = [];
        for (let i = 0; i < iterations; i++) {
            const start = process.hrtime.bigint();
            fn();
            const end = process.hrtime.bigint();
            this.times.push(Number(end - start)); // nanoseconds
        }

        return this;
    }

    /**
     * Get statistics
     */
    getStats() {
        const sorted = [...this.times].sort((a, b) => a - b);
        const sum = this.times.reduce((a, b) => a + b, 0);
        const mean = sum / this.times.length;
        
        // Standard deviation
        const squaredDiffs = this.times.map(t => Math.pow(t - mean, 2));
        const stddev = Math.sqrt(squaredDiffs.reduce((a, b) => a + b, 0) / this.times.length);

        return {
            name: this.name,
            iterations: this.times.length,
            mean_ns: mean,
            mean_us: mean / 1000,
            mean_ms: mean / 1000000,
            stddev_ns: stddev,
            min_ns: sorted[0],
            max_ns: sorted[sorted.length - 1],
            median_ns: sorted[Math.floor(sorted.length / 2)],
            p90_ns: sorted[Math.floor(sorted.length * 0.9)],
            p99_ns: sorted[Math.floor(sorted.length * 0.99)]
        };
    }

    /**
     * Print results
     */
    print() {
        const stats = this.getStats();
        const timeStr = stats.mean_us >= 1000 
            ? `${stats.mean_ms.toFixed(3)} ms` 
            : `${stats.mean_us.toFixed(3)} μs`;
        const stddevStr = stats.stddev_ns / 1000 >= 1000
            ? `${(stats.stddev_ns / 1000000).toFixed(3)} ms`
            : `${(stats.stddev_ns / 1000).toFixed(3)} μs`;
        
        console.log(`${this.name.padEnd(45)} ${timeStr.padStart(15)} ± ${stddevStr.padStart(12)}`);
        return stats;
    }
}

/**
 * Run all benchmarks
 */
function runBenchmarks() {
    console.log('='.repeat(80));
    console.log('Rescue Cipher JavaScript Benchmarks');
    console.log('='.repeat(80));
    console.log(`Iterations: ${BENCHMARK_ITERATIONS}, Warmup: ${WARMUP_ITERATIONS}`);
    console.log('-'.repeat(80));

    const results = {};

    // =========================================================================
    // Cipher Construction Benchmark
    // =========================================================================
    console.log('\n--- Cipher Construction ---');
    
    {
        const bench = new Benchmark('RescueCipher_Construction');
        const secret = generateSharedSecret();
        bench.run(() => {
            const cipher = new RescueCipher(secret);
        });
        results['RescueCipher_Construction'] = bench.print();
    }

    // =========================================================================
    // Encryption Benchmarks
    // =========================================================================
    console.log('\n--- Encryption ---');

    // 1 Block (3 elements)
    {
        const bench = new Benchmark('RescueCipher_Encrypt_1Block');
        const secret = generateSharedSecret();
        const cipher = new RescueCipher(secret);
        const nonce = generateNonce();
        const plaintext = generatePlaintext(BLOCK_SIZE);
        
        bench.run(() => {
            cipher.encrypt(plaintext, nonce);
        });
        results['RescueCipher_Encrypt_1Block'] = bench.print();
    }

    // 10 Blocks (30 elements)
    {
        const bench = new Benchmark('RescueCipher_Encrypt_10Blocks');
        const secret = generateSharedSecret();
        const cipher = new RescueCipher(secret);
        const nonce = generateNonce();
        const plaintext = generatePlaintext(10 * BLOCK_SIZE);
        
        bench.run(() => {
            cipher.encrypt(plaintext, nonce);
        });
        results['RescueCipher_Encrypt_10Blocks'] = bench.print();
    }

    // 100 Blocks (300 elements)
    {
        const bench = new Benchmark('RescueCipher_Encrypt_100Blocks');
        const secret = generateSharedSecret();
        const cipher = new RescueCipher(secret);
        const nonce = generateNonce();
        const plaintext = generatePlaintext(100 * BLOCK_SIZE);
        
        bench.run(() => {
            cipher.encrypt(plaintext, nonce);
        });
        results['RescueCipher_Encrypt_100Blocks'] = bench.print();
    }

    // =========================================================================
    // Decryption Benchmarks
    // =========================================================================
    console.log('\n--- Decryption ---');

    // 1 Block
    {
        const bench = new Benchmark('RescueCipher_Decrypt_1Block');
        const secret = generateSharedSecret();
        const cipher = new RescueCipher(secret);
        const nonce = generateNonce();
        const plaintext = generatePlaintext(BLOCK_SIZE);
        const ciphertext = cipher.encrypt(plaintext, nonce);
        
        bench.run(() => {
            cipher.decrypt(ciphertext, nonce);
        });
        results['RescueCipher_Decrypt_1Block'] = bench.print();
    }

    // 10 Blocks
    {
        const bench = new Benchmark('RescueCipher_Decrypt_10Blocks');
        const secret = generateSharedSecret();
        const cipher = new RescueCipher(secret);
        const nonce = generateNonce();
        const plaintext = generatePlaintext(10 * BLOCK_SIZE);
        const ciphertext = cipher.encrypt(plaintext, nonce);
        
        bench.run(() => {
            cipher.decrypt(ciphertext, nonce);
        });
        results['RescueCipher_Decrypt_10Blocks'] = bench.print();
    }

    // =========================================================================
    // Throughput Benchmarks
    // =========================================================================
    console.log('\n--- Throughput (varying sizes) ---');

    const throughputSizes = [1, 4, 16, 64, 128, 256, 512, 1024];
    const throughputResults = [];

    for (const size of throughputSizes) {
        const bench = new Benchmark(`RescueCipher_Throughput/${size}`);
        const secret = generateSharedSecret();
        const cipher = new RescueCipher(secret);
        const nonce = generateNonce();
        const plaintext = generatePlaintext(size);
        
        bench.run(() => {
            cipher.encrypt(plaintext, nonce);
        }, 50, 5); // Fewer iterations for larger sizes
        
        const stats = bench.print();
        
        // Calculate throughput
        const elementsPerSecond = (size * 1e9) / stats.mean_ns;
        const bytesPerSecond = elementsPerSecond * 32; // 32 bytes per field element
        
        throughputResults.push({
            size,
            stats,
            elements_per_second: elementsPerSecond,
            megabytes_per_second: bytesPerSecond / (1024 * 1024)
        });

        results[`RescueCipher_Throughput/${size}`] = {
            ...stats,
            elements_per_second: elementsPerSecond,
            megabytes_per_second: bytesPerSecond / (1024 * 1024)
        };
    }

    // Print throughput summary
    console.log('\n--- Throughput Summary ---');
    console.log('Size (elements)    Elements/sec        MB/sec');
    console.log('-'.repeat(50));
    for (const t of throughputResults) {
        console.log(
            `${String(t.size).padStart(6)}` +
            `${t.elements_per_second.toFixed(0).padStart(20)}` +
            `${t.megabytes_per_second.toFixed(2).padStart(14)}`
        );
    }

    // =========================================================================
    // Round-trip Benchmark
    // =========================================================================
    console.log('\n--- Round-trip (encrypt + decrypt) ---');

    {
        const bench = new Benchmark('RescueCipher_RoundTrip_10Blocks');
        const secret = generateSharedSecret();
        const cipher = new RescueCipher(secret);
        const nonce = generateNonce();
        const plaintext = generatePlaintext(10 * BLOCK_SIZE);
        
        bench.run(() => {
            const ciphertext = cipher.encrypt(plaintext, nonce);
            cipher.decrypt(ciphertext, nonce);
        });
        results['RescueCipher_RoundTrip_10Blocks'] = bench.print();
    }

    // =========================================================================
    // Save results to JSON
    // =========================================================================
    const output = {
        timestamp: new Date().toISOString(),
        platform: 'JavaScript/Node.js',
        node_version: process.version,
        iterations: BENCHMARK_ITERATIONS,
        warmup_iterations: WARMUP_ITERATIONS,
        benchmarks: results
    };

    fs.writeFileSync('data/benchmark_results_js.json', JSON.stringify(output, null, 2));
    console.log('\n' + '='.repeat(80));
    console.log('Results saved to data/benchmark_results_js.json');

    return output;
}

// Run benchmarks
runBenchmarks();
