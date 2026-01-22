#pragma once

/**
 * @file rescue_cipher.hpp
 * @brief Rescue cipher in Counter (CTR) mode.
 *
 * This file provides the RescueCipher class implementing symmetric encryption
 * using the Rescue permutation in CTR mode.
 *
 * See: https://tosc.iacr.org/index.php/ToSC/article/view/8695/8287
 */

#include <rescue/field.hpp>
#include <rescue/rescue_desc.hpp>
#include <rescue/rescue_hash.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace rescue {

/// Block size for Rescue cipher (number of field elements per block)
constexpr size_t RESCUE_CIPHER_BLOCK_SIZE = 5;

/// Nonce size in bytes for CTR mode
constexpr size_t RESCUE_CIPHER_NONCE_SIZE = 16;

/// Shared secret size in bytes
constexpr size_t RESCUE_CIPHER_SECRET_SIZE = 32;

/**
 * @brief Rescue cipher in Counter (CTR) mode.
 *
 * This class provides symmetric encryption/decryption using the Rescue
 * permutation. The cipher key is derived from a shared secret using
 * RescuePrimeHash, following NIST SP 800-56C Option 1.
 *
 * Security:
 * - 128-bit security level
 * - Block size: 5 field elements
 * - Constant-time operations for side-channel resistance
 */
class RescueCipher {
public:
    /**
     * @brief Construct a RescueCipher from a shared secret.
     * @param shared_secret 32-byte shared secret (e.g., from key exchange).
     * @throws std::invalid_argument if shared_secret is not 32 bytes.
     */
    explicit RescueCipher(std::span<const uint8_t, RESCUE_CIPHER_SECRET_SIZE> shared_secret);

    /**
     * @brief Construct a RescueCipher from a shared secret (vector version).
     * @param shared_secret 32-byte shared secret.
     * @throws std::invalid_argument if shared_secret is not 32 bytes.
     */
    explicit RescueCipher(const std::vector<uint8_t>& shared_secret);

    /**
     * @brief Construct a RescueCipher from a shared secret (array version).
     * @param shared_secret 32-byte shared secret.
     */
    explicit RescueCipher(const std::array<uint8_t, RESCUE_CIPHER_SECRET_SIZE>& shared_secret);

    // Default copy/move operations
    RescueCipher(const RescueCipher&) = default;
    RescueCipher(RescueCipher&&) noexcept = default;
    RescueCipher& operator=(const RescueCipher&) = default;
    RescueCipher& operator=(RescueCipher&&) noexcept = default;
    ~RescueCipher() = default;

    // =========================================================================
    // High-level API (serialized)
    // =========================================================================

    /**
     * @brief Encrypt plaintext field elements.
     * @param plaintext The plaintext as field elements.
     * @param nonce 16-byte nonce (must be unique per message).
     * @return Ciphertext as serialized bytes (32 bytes per field element).
     * @throws std::invalid_argument if nonce is not 16 bytes.
     */
    [[nodiscard]] std::vector<std::vector<uint8_t>> encrypt(
        const std::vector<Fp>& plaintext,
        std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce) const;

    /**
     * @brief Decrypt ciphertext to field elements.
     * @param ciphertext The ciphertext as serialized bytes (32 bytes per element).
     * @param nonce 16-byte nonce (must match encryption nonce).
     * @return Decrypted plaintext as field elements.
     * @throws std::invalid_argument if nonce is not 16 bytes or ciphertext format is invalid.
     */
    [[nodiscard]] std::vector<Fp> decrypt(
        const std::vector<std::vector<uint8_t>>& ciphertext,
        std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce) const;

    // =========================================================================
    // Low-level API (raw field elements)
    // =========================================================================

    /**
     * @brief Encrypt plaintext field elements (raw output).
     * @param plaintext The plaintext as field elements.
     * @param nonce 16-byte nonce.
     * @return Ciphertext as field elements.
     */
    [[nodiscard]] std::vector<Fp> encrypt_raw(
        const std::vector<Fp>& plaintext,
        std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce) const;

    /**
     * @brief Decrypt ciphertext field elements (raw input).
     * @param ciphertext The ciphertext as field elements.
     * @param nonce 16-byte nonce.
     * @return Decrypted plaintext as field elements.
     */
    [[nodiscard]] std::vector<Fp> decrypt_raw(
        const std::vector<Fp>& ciphertext,
        std::span<const uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce) const;

    // =========================================================================
    // Convenience overloads
    // =========================================================================

    /**
     * @brief Encrypt with vector nonce.
     */
    [[nodiscard]] std::vector<std::vector<uint8_t>> encrypt(
        const std::vector<Fp>& plaintext,
        const std::vector<uint8_t>& nonce) const;

    /**
     * @brief Decrypt with vector nonce.
     */
    [[nodiscard]] std::vector<Fp> decrypt(
        const std::vector<std::vector<uint8_t>>& ciphertext,
        const std::vector<uint8_t>& nonce) const;

    /**
     * @brief Encrypt raw with vector nonce.
     */
    [[nodiscard]] std::vector<Fp> encrypt_raw(
        const std::vector<Fp>& plaintext,
        const std::vector<uint8_t>& nonce) const;

    /**
     * @brief Decrypt raw with vector nonce.
     */
    [[nodiscard]] std::vector<Fp> decrypt_raw(
        const std::vector<Fp>& ciphertext,
        const std::vector<uint8_t>& nonce) const;

private:
    RescueDesc desc_;

    /**
     * @brief Derive cipher key from shared secret.
     */
    static std::vector<Fp> derive_key(std::span<const uint8_t> shared_secret);

    /**
     * @brief Generate counter values for CTR mode.
     */
    [[nodiscard]] std::vector<Fp> generate_counter(const mpz_class& nonce, size_t n_blocks) const;

    /**
     * @brief Encrypt/decrypt a single block.
     */
    [[nodiscard]] std::vector<Fp> process_block(const std::vector<Fp>& data,
                                                 const std::vector<Fp>& counter) const;
};

/**
 * @brief Generate a random nonce for Rescue cipher.
 * @return 16-byte random nonce.
 */
[[nodiscard]] std::array<uint8_t, RESCUE_CIPHER_NONCE_SIZE> generate_nonce();

}  // namespace rescue
