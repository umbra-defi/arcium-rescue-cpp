/**
 * @file basic_usage.cpp
 * @brief Example demonstrating basic Rescue cipher usage.
 *
 * This example shows how to:
 * 1. Create field elements
 * 2. Use the Rescue-Prime hash function
 * 3. Encrypt and decrypt data with the Rescue cipher
 */

#include <rescue/rescue.hpp>

#include <iomanip>
#include <iostream>

using namespace rescue;

// Helper to print field elements
void print_fp_vector(const std::string& label, const std::vector<Fp>& vec) {
    std::cout << label << ": [";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << vec[i].value();
    }
    std::cout << "]\n";
}

// Helper to print bytes
void print_bytes(const std::string& label, const std::array<uint8_t, 16>& bytes) {
    std::cout << label << ": ";
    for (auto b : bytes) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    std::cout << std::dec << "\n";
}

int main() {
    std::cout << "=== Rescue Cipher C++ Example ===\n\n";

    // =========================================================================
    // 1. Field Element Operations
    // =========================================================================
    std::cout << "--- Field Element Operations ---\n";

    // Create field elements
    Fp a(uint64_t{42});
    Fp b(uint64_t{100});

    std::cout << "a = " << a << "\n";
    std::cout << "b = " << b << "\n";
    std::cout << "a + b = " << (a + b) << "\n";
    std::cout << "a * b = " << (a * b) << "\n";
    std::cout << "a^2 = " << a.square() << "\n";
    std::cout << "a^(-1) = " << a.inv() << "\n";

    // Verify: a * a^(-1) = 1
    Fp product = a * a.inv();
    std::cout << "a * a^(-1) = " << product << " (should be 1)\n";

    // Random field element
    Fp random_elem = Fp::random();
    std::cout << "Random field element: " << random_elem << "\n\n";

    // =========================================================================
    // 2. Rescue-Prime Hash
    // =========================================================================
    std::cout << "--- Rescue-Prime Hash ---\n";

    RescuePrimeHash hasher;

    // Hash a message
    std::vector<Fp> message = {Fp(uint64_t{1}), Fp(uint64_t{2}), Fp(uint64_t{3})};
    print_fp_vector("Message", message);

    auto digest = hasher.digest(message);
    print_fp_vector("Digest", digest);

    std::cout << "Digest length: " << digest.size() << " field elements\n";

    // Hash a different message
    std::vector<Fp> message2 = {Fp(uint64_t{1}), Fp(uint64_t{2}), Fp(uint64_t{4})};  // Changed 3 to 4
    auto digest2 = hasher.digest(message2);

    std::cout << "Different message produces different digest: " 
              << (digest != digest2 ? "Yes" : "No") << "\n\n";

    // =========================================================================
    // 3. Rescue Cipher (Encryption/Decryption)
    // =========================================================================
    std::cout << "--- Rescue Cipher (CTR Mode) ---\n";

    // Generate a shared secret (in practice, this comes from key exchange)
    auto shared_secret = random_bytes<RESCUE_CIPHER_SECRET_SIZE>();
    std::cout << "Generated " << RESCUE_CIPHER_SECRET_SIZE << "-byte shared secret\n";

    // Create cipher instance
    RescueCipher cipher(shared_secret);

    // Generate a nonce (unique per message)
    auto nonce = generate_nonce();
    print_bytes("Nonce", nonce);

    // Prepare plaintext
    std::vector<Fp> plaintext = {
        Fp(uint64_t{42}),
        Fp(uint64_t{1337}),
        Fp(uint64_t{0xDEADBEEF}),
        Fp(uint64_t{12345}),
        Fp(uint64_t{67890})
    };
    print_fp_vector("Plaintext", plaintext);

    // Encrypt
    auto ciphertext_raw = cipher.encrypt_raw(plaintext, nonce);
    print_fp_vector("Ciphertext", ciphertext_raw);

    // Decrypt
    auto decrypted = cipher.decrypt_raw(ciphertext_raw, nonce);
    print_fp_vector("Decrypted", decrypted);

    // Verify roundtrip
    bool roundtrip_ok = (plaintext == decrypted);
    std::cout << "Roundtrip successful: " << (roundtrip_ok ? "Yes" : "No") << "\n\n";

    // =========================================================================
    // 4. Serialized Encryption (for network transmission)
    // =========================================================================
    std::cout << "--- Serialized Encryption ---\n";

    // Encrypt with serialization
    auto ciphertext_serialized = cipher.encrypt(plaintext, nonce);
    std::cout << "Serialized ciphertext size: " << ciphertext_serialized.size() 
              << " elements x " << Fp::BYTES << " bytes = " 
              << ciphertext_serialized.size() * Fp::BYTES << " bytes total\n";

    // Decrypt from serialized format
    auto decrypted_from_serial = cipher.decrypt(ciphertext_serialized, nonce);
    roundtrip_ok = (plaintext == decrypted_from_serial);
    std::cout << "Serialized roundtrip successful: " << (roundtrip_ok ? "Yes" : "No") << "\n\n";

    // =========================================================================
    // 5. Multiple Blocks
    // =========================================================================
    std::cout << "--- Multi-Block Encryption ---\n";

    // Create longer message (will span multiple blocks)
    std::vector<Fp> long_plaintext;
    for (size_t i = 0; i < 17; ++i) {  // 17 elements = 4 blocks (5 elements each)
        long_plaintext.push_back(Fp(static_cast<uint64_t>(i * 100)));
    }
    std::cout << "Long plaintext: " << long_plaintext.size() << " elements\n";
    std::cout << "Block size: " << RESCUE_CIPHER_BLOCK_SIZE << " elements\n";
    std::cout << "Number of blocks: " << (long_plaintext.size() + RESCUE_CIPHER_BLOCK_SIZE - 1) / RESCUE_CIPHER_BLOCK_SIZE << "\n";

    auto new_nonce = generate_nonce();
    auto long_ct = cipher.encrypt_raw(long_plaintext, new_nonce);
    auto long_dec = cipher.decrypt_raw(long_ct, new_nonce);

    roundtrip_ok = (long_plaintext == long_dec);
    std::cout << "Multi-block roundtrip successful: " << (roundtrip_ok ? "Yes" : "No") << "\n\n";

    // =========================================================================
    // 6. Security Note
    // =========================================================================
    std::cout << "--- Security Note ---\n";
    std::cout << "⚠️  IMPORTANT: Never reuse a nonce with the same key!\n";
    std::cout << "Always generate a fresh nonce for each message.\n\n";

    // Demonstrate nonce reuse danger
    auto nonce_reuse = generate_nonce();
    std::vector<Fp> msg1 = {Fp(uint64_t{100})};
    std::vector<Fp> msg2 = {Fp(uint64_t{200})};

    auto ct1 = cipher.encrypt_raw(msg1, nonce_reuse);
    auto ct2 = cipher.encrypt_raw(msg2, nonce_reuse);

    // If attacker knows msg1, they can derive msg2 from ct1 XOR ct2
    std::cout << "Same nonce, different messages - ciphertexts are related!\n";
    std::cout << "ct1[0] XOR ct2[0] = " << (ct1[0] - ct2[0]) << "\n";
    std::cout << "msg1[0] - msg2[0] = " << (msg1[0] - msg2[0]) << "\n";
    std::cout << "(These are equal - this is why nonce reuse is dangerous!)\n\n";

    std::cout << "=== Example Complete ===\n";
    return 0;
}
