#pragma once

/**
 * @file utils.hpp
 * @brief Utility functions for serialization, random generation, and SHAKE256.
 */

#include <rescue/detail/uint256.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rescue {

// Forward declaration
class Fp;

// ============================================================================
// Serialization utilities
// ============================================================================

/**
 * @brief Serialize a uint256 to little-endian bytes.
 * @param value The value to serialize.
 * @param length_in_bytes The desired output length.
 * @return Byte array of the specified length.
 * @throws std::overflow_error if value is too large for the specified length.
 */
[[nodiscard]] std::vector<uint8_t> serialize_le(const uint256& value, size_t length_in_bytes);

/**
 * @brief Deserialize little-endian bytes to a uint256.
 * @param bytes The bytes to deserialize.
 * @return The deserialized uint256.
 */
[[nodiscard]] uint256 deserialize_le(std::span<const uint8_t> bytes);

// ============================================================================
// Random generation
// ============================================================================

/**
 * @brief Generate cryptographically secure random bytes.
 * @param length Number of bytes to generate.
 * @return Vector of random bytes.
 */
[[nodiscard]] std::vector<uint8_t> random_bytes(size_t length);

/**
 * @brief Generate cryptographically secure random bytes (fixed size).
 * @tparam N Number of bytes.
 * @return Array of N random bytes.
 */
template <size_t N>
[[nodiscard]] std::array<uint8_t, N> random_bytes() {
    std::array<uint8_t, N> result;
    auto vec = random_bytes(N);
    std::copy(vec.begin(), vec.end(), result.begin());
    return result;
}

/**
 * @brief Generate a random uint256 uniformly in [0, bound).
 * @param bound The exclusive upper bound.
 * @return Random value in [0, bound).
 */
[[nodiscard]] uint256 random_field_elem(const uint256& bound);

// ============================================================================
// SHAKE256 Extendable Output Function (XOF)
// ============================================================================

/**
 * @brief SHAKE256 hasher with extendable output.
 *
 * This class wraps OpenSSL's SHAKE256 implementation to provide
 * the extendable output function (XOF) needed for round constant generation.
 */
class Shake256 {
public:
    /**
     * @brief Construct a new SHAKE256 hasher.
     */
    Shake256();

    /**
     * @brief Destructor.
     */
    ~Shake256();

    // Non-copyable, movable
    Shake256(const Shake256&) = delete;
    Shake256& operator=(const Shake256&) = delete;
    Shake256(Shake256&& other) noexcept;
    Shake256& operator=(Shake256&& other) noexcept;

    /**
     * @brief Update the hasher with data.
     * @param data The data to absorb.
     */
    void update(std::span<const uint8_t> data);

    /**
     * @brief Update the hasher with a string.
     * @param str The string to absorb.
     */
    void update(std::string_view str);

    /**
     * @brief Squeeze output bytes from the XOF.
     * @param length Number of bytes to squeeze.
     * @return The squeezed bytes.
     *
     * @note After calling xof(), update() must not be called.
     */
    [[nodiscard]] std::vector<uint8_t> xof(size_t length);

    /**
     * @brief Finalize and get a fixed-length digest.
     * @param length Digest length in bytes.
     * @return The digest.
     */
    [[nodiscard]] std::vector<uint8_t> finalize(size_t length = 32);

private:
    void* ctx_;       // EVP_MD_CTX*
    bool finalized_;  // Whether we've started squeezing
};

/**
 * @brief Compute SHAKE256 hash of data.
 * @param data Input data.
 * @param output_length Desired output length.
 * @return The hash output.
 */
[[nodiscard]] std::vector<uint8_t> shake256(std::span<const uint8_t> data, size_t output_length);

/**
 * @brief Compute SHAKE256 hash of a string.
 * @param str Input string.
 * @param output_length Desired output length.
 * @return The hash output.
 */
[[nodiscard]] std::vector<uint8_t> shake256(std::string_view str, size_t output_length);

// ============================================================================
// SHA-256
// ============================================================================

/**
 * @brief Compute SHA-256 hash.
 * @param data Input data.
 * @return 32-byte hash.
 */
[[nodiscard]] std::array<uint8_t, 32> sha256(std::span<const uint8_t> data);

/**
 * @brief Compute SHA-256 hash of multiple data chunks.
 * @param chunks Vector of data chunks.
 * @return 32-byte hash.
 */
[[nodiscard]] std::array<uint8_t, 32> sha256(const std::vector<std::span<const uint8_t>>& chunks);

}  // namespace rescue
