/**
 * JavaScript Test Vector Generator for Rescue Cipher Interoperability
 * 
 * This script generates test vectors using @arcium-hq/client's RescueCipher
 * and outputs them to a JSON file for verification against the C++ implementation.
 */

const crypto = require('crypto');
const fs = require('fs');
const { RescueCipher } = require('@arcium-hq/client');

// Configuration
const NUM_TEST_CASES = 100;
const MAX_PLAINTEXT_LENGTH = 250; // Maximum number of field elements per test

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
 * Generate random plaintext values as bigints within the field
 * Field modulus: p = 2^255 - 19
 */
function generatePlaintext(length) {
    const p = 2n ** 255n - 19n;
    const plaintext = [];
    
    for (let i = 0; i < length; i++) {
        // Generate a random value less than p
        const bytes = crypto.randomBytes(32);
        let value = 0n;
        for (let j = 0; j < 32; j++) {
            value |= BigInt(bytes[j]) << BigInt(j * 8);
        }
        // Reduce modulo p to ensure it's within the field
        value = value % p;
        plaintext.push(value);
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
 * Main function to generate test vectors
 */
function generateTestVectors() {
    const testVectors = [];
    
    for (let i = 0; i < NUM_TEST_CASES; i++) {
        // Generate random inputs
        const sharedSecret = generateSharedSecret();
        const nonce = generateNonce();
        const plaintextLength = Math.floor(Math.random() * MAX_PLAINTEXT_LENGTH) + 1;
        const plaintext = generatePlaintext(plaintextLength);
        
        // Create cipher and encrypt
        const cipher = new RescueCipher(sharedSecret);
        const ciphertext = cipher.encrypt(plaintext, nonce);
        
        // Decrypt to verify round-trip
        const decrypted = cipher.decrypt(ciphertext, nonce);
        
        // Verify decryption matches original
        for (let j = 0; j < plaintext.length; j++) {
            if (plaintext[j] !== decrypted[j]) {
                throw new Error(`Round-trip verification failed at index ${j}`);
            }
        }
        
        // Store test vector
        testVectors.push({
            id: i,
            shared_secret: toHex(sharedSecret),
            nonce: toHex(nonce),
            plaintext: plaintext.map(bigintToHex),
            ciphertext: ciphertext.map(c => toHex(Uint8Array.from(c))),
            // Also store the raw ciphertext as bigints for easier C++ parsing
            ciphertext_bigints: ciphertext.map(c => {
                let val = 0n;
                for (let k = 0; k < c.length; k++) {
                    val |= BigInt(c[k]) << BigInt(k * 8);
                }
                return bigintToHex(val);
            })
        });
        
        console.log(`Generated test vector ${i + 1}/${NUM_TEST_CASES} (${plaintextLength} elements)`);
    }
    
    // Write to file
    const output = {
        description: "Rescue Cipher Test Vectors for C++ Interoperability",
        field_modulus: bigintToHex(2n ** 255n - 19n),
        num_tests: NUM_TEST_CASES,
        test_vectors: testVectors
    };
    
    fs.writeFileSync('test_vectors_js.json', JSON.stringify(output, null, 2));
    console.log(`\nWritten ${NUM_TEST_CASES} test vectors to test_vectors_js.json`);
}

// Run
generateTestVectors();
