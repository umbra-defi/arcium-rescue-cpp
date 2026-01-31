/**
 * Probe the @arcium-hq/client RescueCipher to discover its parameters
 */

const { RescueCipher } = require('@arcium-hq/client');
const crypto = require('crypto');

// Field modulus
const P = 2n ** 255n - 19n;

function toHex(arr) {
    return Array.from(arr).map(b => b.toString(16).padStart(2, '0')).join('');
}

function bigintToHex(val) {
    const bytes = [];
    let temp = val;
    for (let i = 0; i < 32; i++) {
        bytes.push(Number(temp & 0xffn));
        temp >>= 8n;
    }
    return bytes.map(b => b.toString(16).padStart(2, '0')).join('');
}

console.log('=== Probing @arcium-hq/client RescueCipher ===\n');

// Create a cipher with a known key
const secret = Buffer.alloc(32, 0); // All zeros
for (let i = 0; i < 32; i++) secret[i] = i;
console.log('Shared secret (hex):', toHex(secret));

const cipher = new RescueCipher(secret);

// Check if we can access internal properties
console.log('\nCipher object keys:', Object.keys(cipher));
console.log('Cipher prototype:', Object.getOwnPropertyNames(Object.getPrototypeOf(cipher)));

// Try to access desc or internal state
if (cipher.desc) {
    console.log('\ncipher.desc:', cipher.desc);
}
if (cipher._desc) {
    console.log('\ncipher._desc:', cipher._desc);
}

// Test with different plaintext sizes to determine block size
console.log('\n=== Testing Block Size ===');
const nonce = Buffer.alloc(16, 0);

for (let size = 1; size <= 10; size++) {
    const plaintext = [];
    for (let i = 0; i < size; i++) {
        plaintext.push(BigInt(i + 1));
    }
    
    const ciphertext = cipher.encrypt(plaintext, nonce);
    console.log(`Plaintext size: ${size}, Ciphertext size: ${ciphertext.length}`);
}

// Test with simple inputs
console.log('\n=== Test Vectors with Simple Inputs ===');

// Test 1: Single element [1]
{
    const pt = [1n];
    const ct = cipher.encrypt(pt, nonce);
    console.log('\nPlaintext: [1]');
    console.log('Ciphertext length:', ct.length);
    console.log('Ciphertext[0]:', ct[0] ? (typeof ct[0] === 'bigint' ? bigintToHex(ct[0]) : toHex(ct[0])) : 'undefined');
}

// Test 2: Three elements [1, 2, 3]
{
    const pt = [1n, 2n, 3n];
    const ct = cipher.encrypt(pt, nonce);
    console.log('\nPlaintext: [1, 2, 3]');
    console.log('Ciphertext length:', ct.length);
    for (let i = 0; i < ct.length; i++) {
        const val = ct[i];
        if (typeof val === 'bigint') {
            console.log(`  [${i}]: ${bigintToHex(val)}`);
        } else if (val instanceof Uint8Array || Array.isArray(val)) {
            // Convert bytes to bigint
            let bigVal = 0n;
            for (let j = 0; j < val.length; j++) {
                bigVal |= BigInt(val[j]) << BigInt(j * 8);
            }
            console.log(`  [${i}]: ${bigintToHex(bigVal)} (from bytes)`);
        } else {
            console.log(`  [${i}]: ${val} (type: ${typeof val})`);
        }
    }
}

// Test 3: Five elements [1, 2, 3, 4, 5]
{
    const pt = [1n, 2n, 3n, 4n, 5n];
    const ct = cipher.encrypt(pt, nonce);
    console.log('\nPlaintext: [1, 2, 3, 4, 5]');
    console.log('Ciphertext length:', ct.length);
}

// Try to decrypt and verify
console.log('\n=== Round-trip Test ===');
{
    const pt = [1n, 2n, 3n];
    const ct = cipher.encrypt(pt, nonce);
    const dec = cipher.decrypt(ct, nonce);
    console.log('Original:', pt.map(x => x.toString()));
    console.log('Decrypted:', dec.map(x => x.toString()));
    console.log('Match:', pt.every((v, i) => v === dec[i]));
}

// Check what type the ciphertext elements are
console.log('\n=== Ciphertext Element Type Analysis ===');
{
    const pt = [1n];
    const ct = cipher.encrypt(pt, nonce);
    const elem = ct[0];
    console.log('Type:', typeof elem);
    console.log('Is Array:', Array.isArray(elem));
    console.log('Is Uint8Array:', elem instanceof Uint8Array);
    if (elem instanceof Uint8Array || Array.isArray(elem)) {
        console.log('Length:', elem.length);
        console.log('First few bytes:', Array.from(elem.slice(0, 8)));
    }
}

console.log('\n=== Done ===');
