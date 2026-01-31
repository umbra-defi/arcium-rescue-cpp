#pragma once

/**
 * @file rescue_hash.hpp
 * @brief Rescue-Prime hash function.
 *
 * This file provides the RescuePrimeHash class implementing the sponge-based
 * hash function as described in https://eprint.iacr.org/2020/1143.pdf
 *
 * The hash offers 256 bits of security against collision, preimage, and
 * second-preimage attacks for any field of size at least 102 bits.
 */

#include <rescue/field.hpp>
#include <rescue/rescue_desc.hpp>

#include <cstddef>
#include <vector>

namespace rescue {

/// Default rate for Rescue-Prime hash (absorption rate per permutation)
constexpr size_t RESCUE_HASH_RATE = 7;

/// Default capacity for Rescue-Prime hash (hidden state)
constexpr size_t RESCUE_HASH_CAPACITY = 5;

/// Default state size (rate + capacity)
constexpr size_t RESCUE_HASH_STATE_SIZE = RESCUE_HASH_RATE + RESCUE_HASH_CAPACITY;

/// Default digest length (number of field elements)
constexpr size_t RESCUE_HASH_DIGEST_LENGTH = 5;

/**
 * @brief Rescue-Prime hash function using sponge construction.
 *
 * This implements Algorithm 1 from https://eprint.iacr.org/2020/1143.pdf
 * with padding as described in Algorithm 2.
 *
 * Parameters:
 * - State size (m): 12
 * - Rate: 7
 * - Capacity: 5
 * - Digest length: 5 field elements
 *
 * Security:
 * - 256-bit security against collision attacks
 * - 256-bit security against preimage attacks
 * - 256-bit security against second-preimage attacks
 */
class RescuePrimeHash {
public:
    /**
     * @brief Construct a RescuePrimeHash with default parameters.
     */
    RescuePrimeHash();

    /**
     * @brief Construct a RescuePrimeHash with custom parameters.
     * @param rate The absorption rate (must be positive).
     * @param capacity The capacity (must be positive).
     * @param digest_length The output length in field elements.
     */
    RescuePrimeHash(size_t rate, size_t capacity, size_t digest_length);

    // Default copy/move operations
    RescuePrimeHash(const RescuePrimeHash&) = default;
    RescuePrimeHash(RescuePrimeHash&&) noexcept = default;
    RescuePrimeHash& operator=(const RescuePrimeHash&) = default;
    RescuePrimeHash& operator=(RescuePrimeHash&&) noexcept = default;
    ~RescuePrimeHash() = default;

    /**
     * @brief Compute the hash of a message.
     * @param message The input message as field elements.
     * @return The hash digest as field elements.
     */
    [[nodiscard]] std::vector<Fp> digest(const std::vector<Fp>& message) const;

    /**
     * @brief Compute the hash of a message given as uint256 values.
     * @param message The input message as uint256 values.
     * @return The hash digest as field elements.
     */
    [[nodiscard]] std::vector<Fp> digest(const std::vector<uint256>& message) const;

    /**
     * @brief Get the rate parameter.
     */
    [[nodiscard]] size_t rate() const { return rate_; }

    /**
     * @brief Get the capacity parameter.
     */
    [[nodiscard]] size_t capacity() const { return capacity_; }

    /**
     * @brief Get the digest length.
     */
    [[nodiscard]] size_t digest_length() const { return digest_length_; }

    /**
     * @brief Get the state size (rate + capacity).
     */
    [[nodiscard]] size_t state_size() const { return desc_.m(); }

private:
    size_t rate_;
    size_t capacity_;
    size_t digest_length_;
    RescueDesc desc_;
};

}  // namespace rescue
