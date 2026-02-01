/**
 * Test SHAKE256 output to compare with C++
 */

const { shake256 } = require('@noble/hashes/sha3.js');

function toHex(arr) {
    return Array.from(arr).map(b => b.toString(16).padStart(2, '0')).join('');
}

console.log('=== SHAKE256 Test (JavaScript) ===\n');

// Test with the same initialization as cipher mode
const hasher = shake256.create({ dkLen: 256 / 8 });
hasher.update(new TextEncoder().encode('encrypt everything, compute anything'));

// Extract 48 bytes at a time (like JS library does)
console.log('First 3 XOF outputs (48 bytes each):\n');

const out1 = hasher.xof(48);
console.log('xof(48) #1:', toHex(out1));

const out2 = hasher.xof(48);
console.log('xof(48) #2:', toHex(out2));

const out3 = hasher.xof(48);
console.log('xof(48) #3:', toHex(out3));

// Also show what the first few field elements would be
const P = 2n ** 255n - 19n;

function deserializeLE(bytes) {
    let value = 0n;
    for (let i = 0; i < bytes.length; i++) {
        value |= BigInt(bytes[i]) << BigInt(i * 8);
    }
    return value;
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

// Start fresh to get field elements
const hasher2 = shake256.create({ dkLen: 256 / 8 });
hasher2.update(new TextEncoder().encode('encrypt everything, compute anything'));

console.log('\nFirst 5 field elements from XOF:\n');
for (let i = 0; i < 5; i++) {
    const bytes = hasher2.xof(48);
    const value = deserializeLE(bytes) % P;
    console.log(`  elem[${i}] = ${bigintToHex(value)}`);
}

console.log('\n=== Done ===');
