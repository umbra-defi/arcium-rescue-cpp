/**
 * 100k Benchmark and Interop Test for Rescue Cipher (JavaScript)
 * 
 * Generates 100k test vectors, benchmarks encryption/decryption,
 * and outputs results for comparison with C++ implementation.
 */

const crypto = require('crypto');
const fs = require('fs');
const { RescueCipher } = require('@arcium-hq/client');

// Configuration
const NUM_TEST_CASES = 100000;
const BATCH_SIZE = 1000;  // Smaller batches for more frequent progress updates
const LOG_INTERVAL = 100; // Log every N tests within a batch

// Field modulus
const P = 2n ** 255n - 19n;

/**
 * Get current timestamp string
 */
function timestamp() {
    return new Date().toISOString().substr(11, 12);
}

/**
 * Log with timestamp
 */
function log(msg) {
    console.log(`[${timestamp()}] ${msg}`);
}

/**
 * Generate a deterministic shared secret based on seed
 */
function generateSharedSecret(seed) {
    const hash = crypto.createHash('sha256');
    hash.update(Buffer.from(`secret_${seed}`));
    return hash.digest();
}

/**
 * Generate a deterministic nonce based on seed
 */
function generateNonce(seed) {
    const hash = crypto.createHash('sha256');
    hash.update(Buffer.from(`nonce_${seed}`));
    return hash.digest().slice(0, 16);
}

/**
 * Generate deterministic plaintext based on seed
 */
function generatePlaintext(seed, length) {
    const plaintext = [];
    for (let i = 0; i < length; i++) {
        const hash = crypto.createHash('sha256');
        hash.update(Buffer.from(`plaintext_${seed}_${i}`));
        const bytes = hash.digest();
        let value = 0n;
        for (let j = 0; j < 32; j++) {
            value |= BigInt(bytes[j]) << BigInt(j * 8);
        }
        plaintext.push(value % P);
    }
    return plaintext;
}

/**
 * Convert Uint8Array to hex string
 */
function toHex(arr) {
    return Array.from(arr).map(b => b.toString(16).padStart(2, '0')).join('');
}

/**
 * Convert bigint to hex string (little-endian, 32 bytes)
 */
function bigintToHex(val) {
    const bytes = [];
    let temp = val;
    for (let i = 0; i < 32; i++) {
        bytes.push(Number(temp & 0xffn));
        temp >>= 8n;
    }
    return bytes.map(b => b.toString(16).padStart(2, '0')).join('');
}

/**
 * Format bytes to human readable
 */
function formatBytes(bytes) {
    if (bytes < 1024) return `${bytes} B`;
    if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
    if (bytes < 1024 * 1024 * 1024) return `${(bytes / 1024 / 1024).toFixed(1)} MB`;
    return `${(bytes / 1024 / 1024 / 1024).toFixed(2)} GB`;
}

/**
 * Main benchmark function
 */
async function runBenchmark() {
    const startTime = process.hrtime.bigint();
    
    console.log('');
    console.log('='.repeat(80));
    console.log('  RESCUE CIPHER - JavaScript 100k Benchmark + Interop Test');
    console.log('='.repeat(80));
    log(`Starting benchmark`);
    log(`Configuration:`);
    log(`  - Test cases: ${NUM_TEST_CASES.toLocaleString()}`);
    log(`  - Batch size: ${BATCH_SIZE.toLocaleString()}`);
    log(`  - Field: p = 2^255 - 19`);
    log(`  - Node.js version: ${process.version}`);
    console.log('-'.repeat(80));

    const testVectors = [];
    let totalEncryptTime = 0n;
    let totalDecryptTime = 0n;
    let totalElements = 0;
    let totalCipherConstruction = 0n;
    let totalInputGeneration = 0n;
    let totalVerification = 0n;

    // Plaintext lengths: mix of sizes from 1 to 50 elements
    const getLengthForSeed = (seed) => ((seed * 7) % 50) + 1;

    const numBatches = Math.ceil(NUM_TEST_CASES / BATCH_SIZE);
    
    console.log('');
    log('═══════════════════════════════════════════════════════════════════════════════');
    log('PHASE 1: Test Vector Generation & Benchmarking');
    log('═══════════════════════════════════════════════════════════════════════════════');
    console.log('');

    for (let batch = 0; batch < numBatches; batch++) {
        const batchStart = batch * BATCH_SIZE;
        const batchEnd = Math.min(batchStart + BATCH_SIZE, NUM_TEST_CASES);
        const batchSize = batchEnd - batchStart;
        
        const batchStartTime = process.hrtime.bigint();
        let batchEncryptTime = 0n;
        let batchDecryptTime = 0n;
        let batchElements = 0;
        let batchCipherConstruction = 0n;
        let batchInputGeneration = 0n;
        let batchVerification = 0n;

        log(`Batch ${batch + 1}/${numBatches} starting (tests ${batchStart.toLocaleString()}-${(batchEnd-1).toLocaleString()})`);

        for (let i = batchStart; i < batchEnd; i++) {
            // Generate inputs
            const inputGenStart = process.hrtime.bigint();
            const sharedSecret = generateSharedSecret(i);
            const nonce = generateNonce(i);
            const length = getLengthForSeed(i);
            const plaintext = generatePlaintext(i, length);
            const inputGenEnd = process.hrtime.bigint();
            batchInputGeneration += inputGenEnd - inputGenStart;

            // Create cipher
            const cipherStart = process.hrtime.bigint();
            const cipher = new RescueCipher(sharedSecret);
            const cipherEnd = process.hrtime.bigint();
            batchCipherConstruction += cipherEnd - cipherStart;

            // Benchmark encryption
            const encryptStart = process.hrtime.bigint();
            const ciphertext = cipher.encrypt(plaintext, nonce);
            const encryptEnd = process.hrtime.bigint();
            batchEncryptTime += encryptEnd - encryptStart;

            // Benchmark decryption
            const decryptStart = process.hrtime.bigint();
            const decrypted = cipher.decrypt(ciphertext, nonce);
            const decryptEnd = process.hrtime.bigint();
            batchDecryptTime += decryptEnd - decryptStart;

            // Verify round-trip
            const verifyStart = process.hrtime.bigint();
            for (let j = 0; j < plaintext.length; j++) {
                if (plaintext[j] !== decrypted[j]) {
                    log(`ERROR: Round-trip failed at test ${i}, index ${j}`);
                    log(`  Expected: ${plaintext[j]}`);
                    log(`  Got:      ${decrypted[j]}`);
                    process.exit(1);
                }
            }
            const verifyEnd = process.hrtime.bigint();
            batchVerification += verifyEnd - verifyStart;

            batchElements += length;

            // Store test vector
            testVectors.push({
                id: i,
                shared_secret: toHex(sharedSecret),
                nonce: toHex(nonce),
                plaintext: plaintext.map(bigintToHex),
                ciphertext: ciphertext.map(c => {
                    let val = 0n;
                    for (let k = 0; k < c.length; k++) {
                        val |= BigInt(c[k]) << BigInt(k * 8);
                    }
                    return bigintToHex(val);
                })
            });

            // Progress within batch
            const withinBatch = i - batchStart + 1;
            if (withinBatch % LOG_INTERVAL === 0 || i === batchEnd - 1) {
                const pctBatch = ((withinBatch / batchSize) * 100).toFixed(0);
                const pctTotal = (((i + 1) / NUM_TEST_CASES) * 100).toFixed(1);
                process.stdout.write(`\r  [${timestamp()}]   Progress: ${withinBatch}/${batchSize} (${pctBatch}%) | Total: ${(i+1).toLocaleString()}/${NUM_TEST_CASES.toLocaleString()} (${pctTotal}%)`);
            }
        }

        console.log(''); // New line after progress

        totalEncryptTime += batchEncryptTime;
        totalDecryptTime += batchDecryptTime;
        totalElements += batchElements;
        totalCipherConstruction += batchCipherConstruction;
        totalInputGeneration += batchInputGeneration;
        totalVerification += batchVerification;

        const batchEndTime = process.hrtime.bigint();
        const batchTimeMs = Number(batchEndTime - batchStartTime) / 1e6;
        const progress = ((batch + 1) / numBatches * 100).toFixed(1);
        
        log(`Batch ${batch + 1}/${numBatches} complete:`);
        log(`  - Time: ${batchTimeMs.toFixed(0)}ms`);
        log(`  - Elements: ${batchElements.toLocaleString()}`);
        log(`  - Input gen: ${(Number(batchInputGeneration) / 1e6).toFixed(1)}ms`);
        log(`  - Cipher init: ${(Number(batchCipherConstruction) / 1e6).toFixed(1)}ms`);
        log(`  - Encryption: ${(Number(batchEncryptTime) / 1e6).toFixed(1)}ms`);
        log(`  - Decryption: ${(Number(batchDecryptTime) / 1e6).toFixed(1)}ms`);
        log(`  - Verification: ${(Number(batchVerification) / 1e6).toFixed(1)}ms`);
        log(`  - Overall progress: ${progress}%`);
        console.log('');
    }

    // Calculate statistics
    const avgEncryptTimeUs = Number(totalEncryptTime) / NUM_TEST_CASES / 1000;
    const avgDecryptTimeUs = Number(totalDecryptTime) / NUM_TEST_CASES / 1000;
    const avgElementsPerTest = totalElements / NUM_TEST_CASES;
    const encryptThroughput = totalElements / (Number(totalEncryptTime) / 1e9);
    const decryptThroughput = totalElements / (Number(totalDecryptTime) / 1e9);

    log('═══════════════════════════════════════════════════════════════════════════════');
    log('PHASE 1 COMPLETE: Benchmark Results');
    log('═══════════════════════════════════════════════════════════════════════════════');
    console.log('');
    log(`Summary Statistics:`);
    log(`  Total test cases:        ${NUM_TEST_CASES.toLocaleString()}`);
    log(`  Total elements:          ${totalElements.toLocaleString()}`);
    log(`  Avg elements/test:       ${avgElementsPerTest.toFixed(1)}`);
    console.log('');
    log(`Timing Breakdown:`);
    log(`  Input generation:        ${(Number(totalInputGeneration) / 1e9).toFixed(3)} s`);
    log(`  Cipher construction:     ${(Number(totalCipherConstruction) / 1e9).toFixed(3)} s`);
    log(`  Total encrypt time:      ${(Number(totalEncryptTime) / 1e9).toFixed(3)} s`);
    log(`  Total decrypt time:      ${(Number(totalDecryptTime) / 1e9).toFixed(3)} s`);
    log(`  Verification time:       ${(Number(totalVerification) / 1e9).toFixed(3)} s`);
    console.log('');
    log(`Per-Operation Averages:`);
    log(`  Avg encrypt time/test:   ${avgEncryptTimeUs.toFixed(3)} μs`);
    log(`  Avg decrypt time/test:   ${avgDecryptTimeUs.toFixed(3)} μs`);
    console.log('');
    log(`Throughput:`);
    log(`  Encrypt throughput:      ${encryptThroughput.toFixed(0)} elements/s`);
    log(`  Decrypt throughput:      ${decryptThroughput.toFixed(0)} elements/s`);
    console.log('');

    // Write test vectors to file
    log('═══════════════════════════════════════════════════════════════════════════════');
    log('PHASE 2: Writing Test Vectors to Disk');
    log('═══════════════════════════════════════════════════════════════════════════════');
    console.log('');
    
    const output = {
        description: "100k Rescue Cipher Test Vectors (JavaScript)",
        field_modulus: bigintToHex(P),
        num_tests: NUM_TEST_CASES,
        benchmark_results: {
            platform: "JavaScript/Node.js",
            node_version: process.version,
            total_tests: NUM_TEST_CASES,
            total_elements: totalElements,
            total_encrypt_time_ns: totalEncryptTime.toString(),
            total_decrypt_time_ns: totalDecryptTime.toString(),
            avg_encrypt_time_us: avgEncryptTimeUs,
            avg_decrypt_time_us: avgDecryptTimeUs,
            encrypt_throughput_elements_per_sec: encryptThroughput,
            decrypt_throughput_elements_per_sec: decryptThroughput
        },
        test_vectors: testVectors
    };

    const vectorsFile = 'test_vectors_100k.ndjson';
    log(`Writing ${testVectors.length.toLocaleString()} test vectors to ${vectorsFile}...`);
    
    const writeStart = process.hrtime.bigint();
    const ws = fs.createWriteStream(vectorsFile);
    
    // First line: metadata
    log(`  Writing metadata header...`);
    ws.write(JSON.stringify({
        description: output.description,
        field_modulus: output.field_modulus,
        num_tests: output.num_tests,
        benchmark_results: output.benchmark_results
    }) + '\n');
    
    // Remaining lines: one test vector per line
    log(`  Writing test vectors...`);
    let written = 0;
    for (const vec of testVectors) {
        ws.write(JSON.stringify(vec) + '\n');
        written++;
        if (written % 10000 === 0) {
            log(`    Written ${written.toLocaleString()}/${testVectors.length.toLocaleString()} vectors (${(written/testVectors.length*100).toFixed(0)}%)`);
        }
    }
    
    await new Promise(resolve => ws.end(resolve));
    const writeEnd = process.hrtime.bigint();
    
    const fileSize = fs.statSync(vectorsFile).size;
    log(`  Completed in ${(Number(writeEnd - writeStart) / 1e6).toFixed(0)}ms`);
    log(`  File size: ${formatBytes(fileSize)}`);
    console.log('');

    // Also write summary JSON
    const summaryFile = 'benchmark_results_100k_js.json';
    log(`Writing summary to ${summaryFile}...`);
    fs.writeFileSync(summaryFile, JSON.stringify({
        description: output.description,
        benchmark_results: output.benchmark_results,
        timestamp: new Date().toISOString()
    }, null, 2));
    log(`  Summary written successfully`);
    console.log('');

    const totalTime = process.hrtime.bigint() - startTime;
    log('═══════════════════════════════════════════════════════════════════════════════');
    log('BENCHMARK COMPLETE');
    log('═══════════════════════════════════════════════════════════════════════════════');
    log(`Total execution time: ${(Number(totalTime) / 1e9).toFixed(2)} seconds`);
    console.log('');

    return output.benchmark_results;
}

// Run
log('Initializing...');
runBenchmark().catch(err => {
    log(`ERROR: ${err.message}`);
    console.error(err);
    process.exit(1);
});
