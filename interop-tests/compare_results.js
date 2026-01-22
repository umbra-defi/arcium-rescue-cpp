/**
 * Comparison Script for Rescue Cipher Interoperability
 * 
 * Compares the test vectors from JavaScript and C++ implementations
 * and produces a detailed report.
 */

const fs = require('fs');

function loadJson(path) {
    return JSON.parse(fs.readFileSync(path, 'utf-8'));
}

function main() {
    const jsVectors = loadJson('test_vectors_js.json');
    const cppResults = loadJson('test_vectors_cpp.json');
    
    console.log('='.repeat(60));
    console.log('Rescue Cipher Interoperability Comparison Report');
    console.log('='.repeat(60));
    console.log();
    
    console.log(`JavaScript test vectors: ${jsVectors.num_tests}`);
    console.log(`C++ tests passed: ${cppResults.passed}`);
    console.log(`C++ tests failed: ${cppResults.failed}`);
    console.log();
    
    let allPassed = true;
    
    for (const cppResult of cppResults.test_results) {
        const jsVector = jsVectors.test_vectors.find(v => v.id === cppResult.id);
        
        if (!jsVector) {
            console.log(`[ERROR] Test ${cppResult.id}: JS vector not found`);
            allPassed = false;
            continue;
        }
        
        const encryptMatch = cppResult.encryption_match;
        const decryptMatch = cppResult.decryption_match;
        
        if (encryptMatch && decryptMatch) {
            console.log(`[PASS] Test ${cppResult.id}: Encryption ✓, Decryption ✓`);
        } else {
            allPassed = false;
            console.log(`[FAIL] Test ${cppResult.id}:`);
            
            if (!encryptMatch) {
                console.log(`  Encryption MISMATCH:`);
                for (let i = 0; i < Math.max(jsVector.ciphertext_bigints.length, cppResult.cpp_ciphertext.length); i++) {
                    const jsVal = jsVector.ciphertext_bigints[i] || '(missing)';
                    const cppVal = cppResult.cpp_ciphertext[i] || '(missing)';
                    if (jsVal !== cppVal) {
                        console.log(`    [${i}] JS:  ${jsVal}`);
                        console.log(`    [${i}] C++: ${cppVal}`);
                    }
                }
            }
            
            if (!decryptMatch) {
                console.log(`  Decryption MISMATCH:`);
                for (let i = 0; i < Math.max(jsVector.plaintext.length, cppResult.cpp_decrypted.length); i++) {
                    const jsVal = jsVector.plaintext[i] || '(missing)';
                    const cppVal = cppResult.cpp_decrypted[i] || '(missing)';
                    if (jsVal !== cppVal) {
                        console.log(`    [${i}] JS:  ${jsVal}`);
                        console.log(`    [${i}] C++: ${cppVal}`);
                    }
                }
            }
        }
    }
    
    console.log();
    console.log('='.repeat(60));
    
    if (allPassed) {
        console.log('RESULT: ALL TESTS PASSED ✓');
        console.log('The C++ and JavaScript implementations are interoperable!');
        process.exit(0);
    } else {
        console.log('RESULT: SOME TESTS FAILED ✗');
        console.log('There are discrepancies between the implementations.');
        process.exit(1);
    }
}

main();
