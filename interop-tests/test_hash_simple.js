/**
 * Test hash function with simple input
 */

const { RescuePrimeHash } = require('@arcium-hq/client');

// We need the proper field constant
const CURVE25519_BASE_FIELD = {
    ORDER: 2n ** 255n - 19n,
    BYTES: 32
};

function bigintToHex(val) {
    const bytes = [];
    let temp = val;
    for (let i = 0; i < 32; i++) {
        bytes.push(Number(temp & 0xffn));
        temp >>= 8n;
    }
    return bytes.map(b => b.toString(16).padStart(2, '0')).join('');
}

// This won't work because RescuePrimeHash needs the full field interface
// Let's use RescueCipher to access the hash indirectly

const { RescueCipher } = require('@arcium-hq/client');

console.log('=== Hash Function Test ===\n');

// Create cipher and access its internal hash
const secret = Buffer.alloc(32, 0);
const cipher = new RescueCipher(secret);

// We can't call the hash directly, but we know the KDF process:
// Key = hash([1, secret, 5])

// Let's compare by using a different approach - just print what the cipher
// uses for its internal state

console.log('Cipher internal desc:');
const desc = cipher.cipher.desc;
console.log('  m:', desc.m);
console.log('  nRounds:', desc.nRounds);
console.log('  alpha:', desc.alpha.toString());
console.log('  alphaInverse:', bigintToHex(desc.alphaInverse));

// Print MDS matrix
console.log('\nMDS Matrix (first row):');
if (desc.mdsMat && desc.mdsMat.data) {
    for (let j = 0; j < desc.m; j++) {
        console.log(`  mds[0][${j}] = ${bigintToHex(desc.mdsMat.data[0][j])}`);
    }
}

// Print some round constants
console.log('\nRound constants (first round key):');
if (desc.roundKeys && desc.roundKeys[0] && desc.roundKeys[0].data) {
    for (let i = 0; i < desc.m; i++) {
        console.log(`  rk0[${i}] = ${bigintToHex(desc.roundKeys[0].data[i][0])}`);
    }
}

console.log('\n=== Done ===');
