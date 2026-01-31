/**
 * Debug comparison - print intermediate values from C++
 */

#include <rescue/rescue.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>

using namespace rescue;

std::string fp_to_hex(const Fp& val) {
    auto arr = val.to_bytes();
    std::stringstream ss;
    for (auto b : arr) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    return ss.str();
}

std::string bytes_to_hex(const uint8_t* data, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; i++) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

int main() {
    std::cout << "=== Debug Comparison (C++) ===\n\n";

    // Test with known inputs
    std::array<uint8_t, 32> secret;
    for (int i = 0; i < 32; i++) secret[i] = static_cast<uint8_t>(i);

    std::cout << "Shared secret (32 bytes):\n";
    std::cout << "  Hex: " << bytes_to_hex(secret.data(), 32) << "\n";
    
    uint256 secret_value = deserialize_le(std::span<const uint8_t>(secret.data(), 32));
    Fp secret_fp(secret_value);
    std::cout << "  As field element: " << fp_to_hex(secret_fp) << "\n";

    // Test the hash function directly
    std::cout << "\n=== Hash Function Test ===\n";
    RescuePrimeHash hasher;

    // Hash the KDF input: [1, secret, 5]
    std::cout << "\nKDF Input for key derivation:\n";
    std::cout << "  [0] counter = 1: " << fp_to_hex(Fp(uint64_t{1})) << "\n";
    std::cout << "  [1] secret  = " << fp_to_hex(secret_fp) << "\n";
    std::cout << "  [2] L = 5:    " << fp_to_hex(Fp(uint64_t{5})) << "\n";

    std::vector<Fp> kdf_input;
    kdf_input.emplace_back(uint64_t{1});
    kdf_input.emplace_back(secret_value);
    kdf_input.emplace_back(uint64_t{5});

    auto derived_key = hasher.digest(kdf_input);

    std::cout << "\nDerived Key (5 elements):\n";
    for (size_t i = 0; i < derived_key.size(); i++) {
        std::cout << "  key[" << i << "] = " << fp_to_hex(derived_key[i]) << "\n";
    }

    // Now test encryption
    std::cout << "\n=== Encryption Test ===\n";
    std::array<uint8_t, 16> nonce = {0};
    std::cout << "Nonce (16 bytes): " << bytes_to_hex(nonce.data(), 16) << "\n";
    
    uint256 nonce_value = deserialize_le(std::span<const uint8_t>(nonce.data(), 16));
    std::cout << "Nonce as field element: " << fp_to_hex(Fp(nonce_value)) << "\n";

    RescueCipher cipher(secret);

    // Counter for block 0: [nonce, 0, 0, 0, 0]
    std::cout << "\nCounter for block 0:\n";
    std::vector<Fp> counter = {
        Fp(nonce_value),
        Fp::ZERO,
        Fp::ZERO,
        Fp::ZERO,
        Fp::ZERO
    };
    for (size_t i = 0; i < counter.size(); i++) {
        std::cout << "  counter[" << i << "] = " << fp_to_hex(counter[i]) << "\n";
    }

    // Single element plaintext
    std::vector<Fp> plaintext = {Fp(uint64_t{1})};
    std::cout << "\nPlaintext: [1]\n";
    std::cout << "  pt[0] = " << fp_to_hex(plaintext[0]) << "\n";

    auto ciphertext = cipher.encrypt_raw(plaintext, nonce);
    std::cout << "\nCiphertext:\n";
    std::cout << "  ct[0] = " << fp_to_hex(ciphertext[0]) << "\n";

    // Decrypt to verify
    auto decrypted = cipher.decrypt_raw(ciphertext, nonce);
    std::cout << "\nDecrypted:\n";
    std::cout << "  dec[0] = " << fp_to_hex(decrypted[0]) << "\n";

    // Calculate what the encrypted counter should be
    // ct = pt + encrypted_counter
    // So encrypted_counter = ct - pt
    Fp enc_counter0 = ciphertext[0] - plaintext[0];
    std::cout << "\nInferred encrypted counter (ct - pt):\n";
    std::cout << "  enc_counter[0] = " << fp_to_hex(enc_counter0) << "\n";

    std::cout << "\n=== Done ===\n";
    return 0;
}
