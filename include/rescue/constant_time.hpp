#pragma once

/**
 * @file constant_time.hpp
 * @brief Constant-time operations for side-channel resistance.
 *
 * This module provides constant-time arithmetic operations using binary
 * representation in 2's complement form. These operations prevent timing
 * side-channel attacks by ensuring execution time is independent of secret data.
 */

#include <cstddef>
#include <cstdint>
#include <vector>

#include <gmpxx.h>

namespace rescue::ct {

/**
 * @brief Convert a bigint to little-endian bit representation (2's complement).
 * @param x The value to convert.
 * @param bin_size The number of bits in the representation.
 * @return Vector of bools representing the bits (LSB first).
 */
[[nodiscard]] std::vector<bool> to_bin_le(const mpz_class& x, size_t bin_size);

/**
 * @brief Convert a little-endian bit array (2's complement) to a bigint.
 * @param x_bin The bit array to convert.
 * @return The bigint value (may be negative if sign bit is set).
 */
[[nodiscard]] mpz_class from_bin_le(const std::vector<bool>& x_bin);

/**
 * @brief Get a specific bit from a bigint.
 * @param x The value.
 * @param bit_pos The bit position (0 = LSB).
 * @return True if the bit is set, false otherwise.
 */
[[nodiscard]] bool get_bit(const mpz_class& x, size_t bit_pos);

/**
 * @brief Get the sign bit of a value in 2's complement representation.
 * @param x The value.
 * @param bin_size The position of the sign bit.
 * @return True if the sign bit is set (value is negative in 2's complement).
 */
[[nodiscard]] bool sign_bit(const mpz_class& x, size_t bin_size);

/**
 * @brief Constant-time binary adder.
 * @param x_bin First operand as bit array.
 * @param y_bin Second operand as bit array (must be same size as x_bin).
 * @param carry_in Initial carry value.
 * @param bin_size Number of bits to process.
 * @return Sum as bit array.
 */
[[nodiscard]] std::vector<bool> adder(const std::vector<bool>& x_bin,
                                      const std::vector<bool>& y_bin,
                                      bool carry_in,
                                      size_t bin_size);

/**
 * @brief Constant-time addition in 2's complement representation.
 * @param x First operand.
 * @param y Second operand.
 * @param bin_size Number of bits for the operation.
 * @return x + y in 2's complement (may overflow/wrap).
 */
[[nodiscard]] mpz_class add(const mpz_class& x, const mpz_class& y, size_t bin_size);

/**
 * @brief Constant-time subtraction in 2's complement representation.
 * @param x First operand.
 * @param y Second operand.
 * @param bin_size Number of bits for the operation.
 * @return x - y in 2's complement (may underflow/wrap).
 */
[[nodiscard]] mpz_class sub(const mpz_class& x, const mpz_class& y, size_t bin_size);

/**
 * @brief Constant-time less-than comparison.
 * @param x First operand.
 * @param y Second operand.
 * @param bin_size Number of bits for the operation.
 * @return True if x < y (as signed integers in 2's complement).
 */
[[nodiscard]] bool lt(const mpz_class& x, const mpz_class& y, size_t bin_size);

/**
 * @brief Constant-time selection between two values.
 * @param cond Condition: if true, return x; if false, return y.
 * @param x Value to return if cond is true.
 * @param y Value to return if cond is false.
 * @param bin_size Number of bits for the operation.
 * @return x if cond is true, y otherwise.
 */
[[nodiscard]] mpz_class select(bool cond, const mpz_class& x, const mpz_class& y, size_t bin_size);

/**
 * @brief Verify that a value fits within the given bit size in 2's complement.
 * @param x The value to check.
 * @param bin_size The bit size to verify against.
 * @return True if x can be represented in bin_size bits (signed).
 *
 * @note This function is constant-time only for inputs that return true.
 */
[[nodiscard]] bool verify_bin_size(const mpz_class& x, size_t bin_size);

/**
 * @brief Calculate the minimum bin_size needed for field operations.
 * @param max_value The maximum value to represent (e.g., field order - 1).
 * @return The bin_size needed for safe constant-time operations.
 */
[[nodiscard]] size_t get_bin_size(const mpz_class& max_value);

/**
 * @brief Constant-time field addition (x + y mod p).
 *
 * This performs addition followed by conditional reduction, all in constant time.
 *
 * @param x First operand (must be in [0, p)).
 * @param y Second operand (must be in [0, p)).
 * @param p The field modulus.
 * @param bin_size Number of bits for the operation.
 * @return (x + y) mod p.
 */
[[nodiscard]] mpz_class field_add(const mpz_class& x,
                                   const mpz_class& y,
                                   const mpz_class& p,
                                   size_t bin_size);

/**
 * @brief Constant-time field subtraction (x - y mod p).
 *
 * This performs subtraction followed by conditional addition of p, all in constant time.
 *
 * @param x First operand (must be in [0, p)).
 * @param y Second operand (must be in [0, p)).
 * @param p The field modulus.
 * @param bin_size Number of bits for the operation.
 * @return (x - y) mod p.
 */
[[nodiscard]] mpz_class field_sub(const mpz_class& x,
                                   const mpz_class& y,
                                   const mpz_class& p,
                                   size_t bin_size);

}  // namespace rescue::ct
