/**
 * Debug comparison - print intermediate values from JS
 */

const { RescueCipher } = require('@arcium-hq/client');

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

function deserializeLE(bytes) {
    let value = 0n;
    for (let i = 0; i < bytes.length; i++) {
        value |= BigInt(bytes[i]) << BigInt(i * 8);
    }
    return value;
}

console.log('=== Debug Comparison (JavaScript) ===\n');

// Test with known inputs
const secret = Buffer.alloc(32, 0);
for (let i = 0; i < 32; i++) secret[i] = i;

console.log('Shared secret (32 bytes):');
console.log('  Hex:', toHex(secret));
console.log('  As field element:', bigintToHex(deserializeLE(secret)));

// Create cipher
const cipher = new RescueCipher(secret);

// Print internal desc info if accessible
console.log('\nCipher internal state:');
if (cipher.cipher && cipher.cipher.desc) {
    const desc = cipher.cipher.desc;
    console.log('  m (state size):', desc.m);
    console.log('  nRounds:', desc.nRounds);
    console.log('  alpha:', desc.alpha ? desc.alpha.toString() : 'N/A');
    console.log('  alphaInverse:', desc.alphaInverse ? bigintToHex(desc.alphaInverse) : 'N/A');
    
    // Print round keys
    if (desc.roundKeys) {
        console.log('  Number of round keys:', desc.roundKeys.length);
        if (desc.roundKeys.length > 0 && desc.roundKeys[0].data) {
            console.log('  First round key:');
            for (let i = 0; i < Math.min(desc.roundKeys[0].data.length, 5); i++) {
                console.log(`    rk[0][${i}] = ${bigintToHex(desc.roundKeys[0].data[i][0])}`);
            }
        }
    }
}

// Now test encryption
console.log('\n=== Encryption Test ===');
const nonce = Buffer.alloc(16, 0);
console.log('Nonce (16 bytes):', toHex(nonce));
console.log('Nonce as field element:', bigintToHex(deserializeLE(nonce)));

// Counter for block 0: [nonce, 0, 0, 0, 0]
const nonceField = deserializeLE(nonce);
console.log('\nCounter for block 0 (expected):');
console.log('  counter[0] = ' + bigintToHex(nonceField) + ' (nonce)');
console.log('  counter[1] = ' + bigintToHex(0n) + ' (block index)');
console.log('  counter[2] = ' + bigintToHex(0n) + ' (padding)');
console.log('  counter[3] = ' + bigintToHex(0n) + ' (padding)');
console.log('  counter[4] = ' + bigintToHex(0n) + ' (padding)');

// Single element plaintext
const plaintext = [1n];
console.log('\nPlaintext: [1]');
console.log('  pt[0] =', bigintToHex(1n));

const ciphertext = cipher.encrypt(plaintext, nonce);
console.log('\nCiphertext:');
const ct0 = ciphertext[0];
let ctValue = 0n;
for (let i = 0; i < ct0.length; i++) {
    ctValue |= BigInt(ct0[i]) << BigInt(i * 8);
}
console.log('  ct[0] =', bigintToHex(ctValue));

// Verify decryption
const decrypted = cipher.decrypt(ciphertext, nonce);
console.log('\nDecrypted:');
console.log('  dec[0] =', bigintToHex(decrypted[0]));
console.log('  Match:', decrypted[0] === 1n);

// We can calculate what the encrypted counter should be:
// ct = pt + encrypted_counter (mod p)
// So encrypted_counter = ct - pt (mod p)
const encCounter0 = (ctValue - 1n + P) % P;
console.log('\nInferred encrypted counter (ct - pt mod p):');
console.log('  enc_counter[0] =', bigintToHex(encCounter0));

// Test with larger plaintext
console.log('\n=== Multi-element Test ===');
const plaintext5 = [1n, 2n, 3n, 4n, 5n];
console.log('Plaintext: [1, 2, 3, 4, 5]');
for (let i = 0; i < 5; i++) {
    console.log(`  pt[${i}] = ${bigintToHex(plaintext5[i])}`);
}

const ciphertext5 = cipher.encrypt(plaintext5, nonce);
console.log('\nCiphertext (5 elements):');
for (let i = 0; i < ciphertext5.length; i++) {
    let val = 0n;
    for (let j = 0; j < ciphertext5[i].length; j++) {
        val |= BigInt(ciphertext5[i][j]) << BigInt(j * 8);
    }
    console.log(`  ct[${i}] = ${bigintToHex(val)}`);
}

console.log('\nInferred encrypted counter values:');
for (let i = 0; i < ciphertext5.length; i++) {
    let val = 0n;
    for (let j = 0; j < ciphertext5[i].length; j++) {
        val |= BigInt(ciphertext5[i][j]) << BigInt(j * 8);
    }
    const enc = (val - plaintext5[i] + P) % P;
    console.log(`  enc_counter[${i}] = ${bigintToHex(enc)}`);
}

console.log('\n=== Done ===');
