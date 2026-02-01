/**
 * Chunked Benchmark - JavaScript
 * 
 * Generates and benchmarks a single chunk of test cases.
 * Called by the orchestrator script for each iteration.
 * 
 * Usage: node chunk_benchmark_js.js <chunk_index> <chunk_size> <output_file>
 */

const crypto = require('crypto');
const fs = require('fs');
const { RescueCipher } = require('@arcium-hq/client');

// Field modulus
const P = 2n ** 255n - 19n;

/**
 * Get current timestamp string
 */
function timestamp() {
    return new Date().toISOString().substr(11, 12);
}

/**
 * Log with timestamp and prefix
 */
function log(msg) {
    console.log(`[${timestamp()}] [JS] ${msg}`);
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
 * Main function
 */
function main() {
    const args = process.argv.slice(2);
    
    if (args.length < 3) {
        console.error('Usage: node chunk_benchmark_js.js <chunk_index> <chunk_size> <output_file>');
        process.exit(1);
    }
    
    const chunkIndex = parseInt(args[0], 10);
    const chunkSize = parseInt(args[1], 10);
    const outputFile = args[2];
    
    const startId = chunkIndex * chunkSize;
    const endId = startId + chunkSize;
    
    log(`Starting chunk ${chunkIndex} (tests ${startId.toLocaleString()}-${(endId-1).toLocaleString()})`);
    log(`  Chunk size: ${chunkSize.toLocaleString()}`);
    log(`  Output file: ${outputFile}`);
    
    const testVectors = [];
    let totalEncryptTime = 0n;
    let totalDecryptTime = 0n;
    let totalElements = 0;
    let totalCipherInit = 0n;
    
    // Plaintext lengths: mix of sizes from 1 to 50 elements
    const getLengthForSeed = (seed) => ((seed * 7) % 50) + 1;
    
    const overallStart = process.hrtime.bigint();
    
    // Generate and benchmark test cases
    log(`Generating ${chunkSize} test cases...`);
    
    for (let i = startId; i < endId; i++) {
        const sharedSecret = generateSharedSecret(i);
        const nonce = generateNonce(i);
        const length = getLengthForSeed(i);
        const plaintext = generatePlaintext(i, length);
        
        // Create cipher
        const cipherStart = process.hrtime.bigint();
        const cipher = new RescueCipher(sharedSecret);
        const cipherEnd = process.hrtime.bigint();
        totalCipherInit += cipherEnd - cipherStart;
        
        // Benchmark encryption
        const encryptStart = process.hrtime.bigint();
        const ciphertext = cipher.encrypt(plaintext, nonce);
        const encryptEnd = process.hrtime.bigint();
        totalEncryptTime += encryptEnd - encryptStart;
        
        // Benchmark decryption
        const decryptStart = process.hrtime.bigint();
        const decrypted = cipher.decrypt(ciphertext, nonce);
        const decryptEnd = process.hrtime.bigint();
        totalDecryptTime += decryptEnd - decryptStart;
        
        // Verify round-trip
        for (let j = 0; j < plaintext.length; j++) {
            if (plaintext[j] !== decrypted[j]) {
                log(`ERROR: Round-trip failed at test ${i}, index ${j}`);
                process.exit(1);
            }
        }
        
        totalElements += length;
        
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
        
        // Progress every 100 tests
        const progress = i - startId + 1;
        if (progress % 100 === 0) {
            const pct = ((progress / chunkSize) * 100).toFixed(0);
            process.stdout.write(`\r[${timestamp()}] [JS]   Progress: ${progress}/${chunkSize} (${pct}%)`);
        }
    }
    console.log(''); // newline after progress
    
    const overallEnd = process.hrtime.bigint();
    const overallTimeMs = Number(overallEnd - overallStart) / 1e6;
    
    // Write test vectors
    log(`Writing ${testVectors.length} test vectors to ${outputFile}...`);
    
    const output = {
        chunk_index: chunkIndex,
        chunk_size: chunkSize,
        start_id: startId,
        end_id: endId,
        total_elements: totalElements,
        test_vectors: testVectors
    };
    
    fs.writeFileSync(outputFile, JSON.stringify(output));
    
    // Calculate and output statistics
    const avgEncryptUs = Number(totalEncryptTime) / chunkSize / 1000;
    const avgDecryptUs = Number(totalDecryptTime) / chunkSize / 1000;
    const encThroughput = totalElements / (Number(totalEncryptTime) / 1e9);
    const decThroughput = totalElements / (Number(totalDecryptTime) / 1e9);
    
    log(`Chunk ${chunkIndex} complete:`);
    log(`  Total time: ${overallTimeMs.toFixed(0)}ms`);
    log(`  Elements: ${totalElements.toLocaleString()}`);
    log(`  Cipher init: ${(Number(totalCipherInit) / 1e6).toFixed(1)}ms`);
    log(`  Encrypt: ${(Number(totalEncryptTime) / 1e6).toFixed(1)}ms (avg: ${avgEncryptUs.toFixed(1)}μs/test)`);
    log(`  Decrypt: ${(Number(totalDecryptTime) / 1e6).toFixed(1)}ms (avg: ${avgDecryptUs.toFixed(1)}μs/test)`);
    log(`  Throughput: ${encThroughput.toFixed(0)} enc/s, ${decThroughput.toFixed(0)} dec/s`);
    
    // Output JSON stats to stdout for orchestrator to parse
    const stats = {
        chunk_index: chunkIndex,
        tests: chunkSize,
        elements: totalElements,
        encrypt_time_ns: totalEncryptTime.toString(),
        decrypt_time_ns: totalDecryptTime.toString(),
        avg_encrypt_us: avgEncryptUs,
        avg_decrypt_us: avgDecryptUs,
        encrypt_throughput: encThroughput,
        decrypt_throughput: decThroughput
    };
    
    // Write stats to separate file for orchestrator
    fs.writeFileSync(outputFile + '.stats.json', JSON.stringify(stats, null, 2));
    
    log(`Stats written to ${outputFile}.stats.json`);
}

main();
