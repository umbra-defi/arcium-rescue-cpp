#include <rescue/rescue_cipher.hpp>

#include <rescue/matrix.hpp>
#include <rescue/utils.hpp>

#include <algorithm>
#include <stdexcept>

namespace rescue {

RescueCipher::RescueCipher(std::span<const uint8_t, RESCUE_CIPHER_SECRET_SIZE> shared_secret)
    : desc_(derive_key(shared_secret)) {}

RescueCipher::RescueCipher(const std::vector<uint8_t>& shared_secret)
    : desc_(derive_key(shared_secret)) {
    if (shared_secret.size() != RESCUE_CIPHER_SECRET_SIZE) {
        throw std::invalid_argument("Shared secret must be " +
                                    std::to_string(RESCUE_CIPHER_SECRET_SIZE) + " bytes");
    }
}

RescueCipher::RescueCipher(const std::array<uint8_t, RESCUE_CIPHER_SECRET_SIZE>& shared_secret)
    : desc_(derive_key(std::span<const uint8_t>(shared_secret.data(), shared_secret.size()))) {}

std::vector<Fp> RescueCipher::derive_key(std::span<const uint8_t> shared_secret) {
    if (shared_secret.size() != RESCUE_CIPHER_SECRET_SIZE) {
        throw std::invalid_argument("Shared secret must be " +
                                    std::to_string(RESCUE_CIPHER_SECRET_SIZE) + " bytes");
    }

    RescuePrimeHash hasher;

    // Convert shared secret to field element
    // For Curve25519 base field, we can fit 32 bytes in one field element
    uint256 secret_value = deserialize_le(shared_secret);

    // Build the counter || Z || FixedInfo vector per NIST SP 800-56C Option 1
    // counter = 1 (we only need one iteration since reps = 1)
    // Z = shared_secret
    // FixedInfo = L = RESCUE_CIPHER_BLOCK_SIZE
    std::vector<Fp> kdf_input;
    kdf_input.emplace_back(uint64_t{1});  // counter
    kdf_input.emplace_back(secret_value);  // Z (shared secret)
    kdf_input.emplace_back(uint64_t{RESCUE_CIPHER_BLOCK_SIZE});  // FixedInfo (L)

    // Hash to derive key
    return hasher.digest(kdf_input);
}

std::vector<std::vector<uint8_t>> RescueCipher::encrypt(
    const std::vector<Fp>& plaintext,
    std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce) const {

    auto raw_ciphertext = encrypt_raw(plaintext, nonce);

    std::vector<std::vector<uint8_t>> result;
    result.reserve(raw_ciphertext.size());

    for (const auto& elem : raw_ciphertext) {
        auto bytes = elem.to_bytes();
        result.emplace_back(bytes.begin(), bytes.end());
    }

    return result;
}

std::vector<Fp> RescueCipher::decrypt(
    const std::vector<std::vector<uint8_t>>& ciphertext,
    std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce) const {

    std::vector<Fp> raw_ciphertext;
    raw_ciphertext.reserve(ciphertext.size());

    for (const auto& bytes : ciphertext) {
        if (bytes.size() != Fp::BYTES) {
            throw std::invalid_argument("Each ciphertext element must be " +
                                        std::to_string(Fp::BYTES) + " bytes");
        }
        raw_ciphertext.emplace_back(std::span<const uint8_t>(bytes.data(), bytes.size()));
    }

    return decrypt_raw(raw_ciphertext, nonce);
}

std::vector<Fp> RescueCipher::encrypt_raw(
    const std::vector<Fp>& plaintext,
    std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce) const {

    if (plaintext.empty()) {
        return {};
    }

    // Calculate number of blocks needed
    size_t n_blocks =
        (plaintext.size() + RESCUE_CIPHER_BLOCK_SIZE - 1) / RESCUE_CIPHER_BLOCK_SIZE;

    // Generate counter values
    uint256 nonce_value = deserialize_le(nonce);
    auto counter = generate_counter(nonce_value, n_blocks);

    std::vector<Fp> ciphertext;
    ciphertext.reserve(plaintext.size());

    for (size_t block = 0; block < n_blocks; ++block) {
        // Extract counter for this block
        std::vector<Fp> block_counter;
        block_counter.reserve(RESCUE_CIPHER_BLOCK_SIZE);
        size_t counter_offset = block * RESCUE_CIPHER_BLOCK_SIZE;
        for (size_t i = 0; i < RESCUE_CIPHER_BLOCK_SIZE; ++i) {
            block_counter.push_back(counter[counter_offset + i]);
        }

        // Encrypt the counter
        Matrix counter_vec(block_counter);
        Matrix encrypted_counter = desc_.permute(counter_vec);
        auto encrypted_counter_data = encrypted_counter.to_vector();

        // XOR (field addition) with plaintext
        size_t block_start = block * RESCUE_CIPHER_BLOCK_SIZE;
        size_t block_end = std::min(block_start + RESCUE_CIPHER_BLOCK_SIZE, plaintext.size());

        for (size_t i = block_start; i < block_end; ++i) {
            size_t idx = i - block_start;
            // Use the optimized field addition (constant-time by design)
            ciphertext.push_back(plaintext[i] + encrypted_counter_data[idx]);
        }
    }

    return ciphertext;
}

std::vector<Fp> RescueCipher::decrypt_raw(
    const std::vector<Fp>& ciphertext,
    std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce) const {

    if (ciphertext.empty()) {
        return {};
    }

    // Calculate number of blocks needed
    size_t n_blocks =
        (ciphertext.size() + RESCUE_CIPHER_BLOCK_SIZE - 1) / RESCUE_CIPHER_BLOCK_SIZE;

    // Generate counter values
    uint256 nonce_value = deserialize_le(nonce);
    auto counter = generate_counter(nonce_value, n_blocks);

    std::vector<Fp> plaintext;
    plaintext.reserve(ciphertext.size());

    for (size_t block = 0; block < n_blocks; ++block) {
        // Extract counter for this block
        std::vector<Fp> block_counter;
        block_counter.reserve(RESCUE_CIPHER_BLOCK_SIZE);
        size_t counter_offset = block * RESCUE_CIPHER_BLOCK_SIZE;
        for (size_t i = 0; i < RESCUE_CIPHER_BLOCK_SIZE; ++i) {
            block_counter.push_back(counter[counter_offset + i]);
        }

        // Encrypt the counter (same as encryption - CTR mode is symmetric)
        Matrix counter_vec(block_counter);
        Matrix encrypted_counter = desc_.permute(counter_vec);
        auto encrypted_counter_data = encrypted_counter.to_vector();

        // XOR (field subtraction) with ciphertext
        size_t block_start = block * RESCUE_CIPHER_BLOCK_SIZE;
        size_t block_end = std::min(block_start + RESCUE_CIPHER_BLOCK_SIZE, ciphertext.size());

        for (size_t i = block_start; i < block_end; ++i) {
            size_t idx = i - block_start;
            // Use the optimized field subtraction (constant-time by design)
            plaintext.push_back(ciphertext[i] - encrypted_counter_data[idx]);
        }
    }

    return plaintext;
}

std::vector<Fp> RescueCipher::generate_counter(const uint256& nonce, size_t n_blocks) const {
    std::vector<Fp> counter;
    counter.reserve(n_blocks * RESCUE_CIPHER_BLOCK_SIZE);

    for (size_t block = 0; block < n_blocks; ++block) {
        // Counter format: [nonce, block_index, 0, 0, ...]
        counter.emplace_back(nonce);
        counter.emplace_back(uint64_t{block});

        // Pad to block size
        for (size_t j = 2; j < RESCUE_CIPHER_BLOCK_SIZE; ++j) {
            counter.push_back(Fp::ZERO);
        }
    }

    return counter;
}

std::vector<Fp> RescueCipher::process_block(const std::vector<Fp>& data,
                                             const std::vector<Fp>& counter) const {
    if (counter.size() != RESCUE_CIPHER_BLOCK_SIZE) {
        throw std::invalid_argument("Counter must have " +
                                    std::to_string(RESCUE_CIPHER_BLOCK_SIZE) + " elements");
    }

    Matrix counter_vec(counter);
    Matrix encrypted_counter = desc_.permute(counter_vec);
    auto encrypted_counter_data = encrypted_counter.to_vector();

    std::vector<Fp> result;
    result.reserve(data.size());

    for (size_t i = 0; i < data.size(); ++i) {
        result.push_back(data[i] + encrypted_counter_data[i]);
    }

    return result;
}

// Convenience overloads

std::vector<std::vector<uint8_t>> RescueCipher::encrypt(
    const std::vector<Fp>& plaintext,
    const std::vector<uint8_t>& nonce) const {
    if (nonce.size() != RESCUE_CIPHER_NONCE_SIZE) {
        throw std::invalid_argument("Nonce must be " +
                                    std::to_string(RESCUE_CIPHER_NONCE_SIZE) + " bytes");
    }
    std::array<uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce_arr;
    std::copy(nonce.begin(), nonce.end(), nonce_arr.begin());
    return encrypt(plaintext, std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE>(nonce_arr));
}

std::vector<Fp> RescueCipher::decrypt(
    const std::vector<std::vector<uint8_t>>& ciphertext,
    const std::vector<uint8_t>& nonce) const {
    if (nonce.size() != RESCUE_CIPHER_NONCE_SIZE) {
        throw std::invalid_argument("Nonce must be " +
                                    std::to_string(RESCUE_CIPHER_NONCE_SIZE) + " bytes");
    }
    std::array<uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce_arr;
    std::copy(nonce.begin(), nonce.end(), nonce_arr.begin());
    return decrypt(ciphertext, std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE>(nonce_arr));
}

std::vector<Fp> RescueCipher::encrypt_raw(
    const std::vector<Fp>& plaintext,
    const std::vector<uint8_t>& nonce) const {
    if (nonce.size() != RESCUE_CIPHER_NONCE_SIZE) {
        throw std::invalid_argument("Nonce must be " +
                                    std::to_string(RESCUE_CIPHER_NONCE_SIZE) + " bytes");
    }
    std::array<uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce_arr;
    std::copy(nonce.begin(), nonce.end(), nonce_arr.begin());
    return encrypt_raw(plaintext, std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE>(nonce_arr));
}

std::vector<Fp> RescueCipher::decrypt_raw(
    const std::vector<Fp>& ciphertext,
    const std::vector<uint8_t>& nonce) const {
    if (nonce.size() != RESCUE_CIPHER_NONCE_SIZE) {
        throw std::invalid_argument("Nonce must be " +
                                    std::to_string(RESCUE_CIPHER_NONCE_SIZE) + " bytes");
    }
    std::array<uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce_arr;
    std::copy(nonce.begin(), nonce.end(), nonce_arr.begin());
    return decrypt_raw(ciphertext, std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE>(nonce_arr));
}

std::array<uint8_t, RESCUE_CIPHER_NONCE_SIZE> generate_nonce() {
    return random_bytes<RESCUE_CIPHER_NONCE_SIZE>();
}

}  // namespace rescue
