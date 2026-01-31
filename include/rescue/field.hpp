#pragma once

/**
 * @file field.hpp
 * @brief Finite field arithmetic over the Curve25519 base field.
 *
 * This file provides the Fp class for field element operations over
 * the prime field p = 2^255 - 19 (Curve25519 base field).
 */

#include <rescue/fp_impl.hpp>
#include <rescue/uint256.hpp>

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

namespace rescue {

/**
 * @brief Field element in F_p where p = 2^255 - 19 (Curve25519 base field).
 *
 * This class provides constant-time arithmetic operations for cryptographic use.
 * All operations are performed modulo the field prime p.
 */
class Fp {
public:
    /// The field modulus p = 2^255 - 19 (as uint256)
    static const uint256 P;

    /// Number of bits in the field modulus
    static constexpr size_t BITS = 255;

    /// Number of bytes needed to represent a field element
    static constexpr size_t BYTES = 32;

    /// Zero element
    static const Fp ZERO;

    /// One element (multiplicative identity)
    static const Fp ONE;

    /**
     * @brief Default constructor, initializes to zero.
     */
    Fp() : value_{} {}

    /**
     * @brief Construct from an unsigned integer.
     * @param value The value (will be reduced modulo p).
     */
    explicit Fp(uint64_t value);

    /**
     * @brief Construct from a uint256.
     * @param value The value (will be reduced modulo p).
     */
    explicit Fp(const uint256& value);

    /**
     * @brief Construct from a uint256 (move).
     * @param value The value (will be reduced modulo p).
     */
    explicit Fp(uint256&& value);

    /**
     * @brief Construct from a hexadecimal string.
     * @param hex_str Hexadecimal string (with or without 0x prefix).
     */
    explicit Fp(std::string_view hex_str);

    /**
     * @brief Construct from a byte array (little-endian).
     * @param bytes The byte array to deserialize.
     */
    explicit Fp(std::span<const uint8_t> bytes);

    // Default copy/move operations
    Fp(const Fp&) = default;
    Fp(Fp&&) noexcept = default;
    Fp& operator=(const Fp&) = default;
    Fp& operator=(Fp&&) noexcept = default;
    ~Fp() = default;

    /**
     * @brief Generate a random field element.
     * @return A uniformly random element in [0, p).
     */
    [[nodiscard]] static Fp random();

    /**
     * @brief Create a field element from a value (reduces mod p).
     * @param value The value to reduce.
     * @return The field element.
     */
    [[nodiscard]] static Fp create(const uint256& value);

    // Arithmetic operations

    /**
     * @brief Add two field elements.
     * @param rhs Right-hand side operand.
     * @return this + rhs (mod p).
     */
    [[nodiscard]] Fp add(const Fp& rhs) const;

    /**
     * @brief Subtract two field elements.
     * @param rhs Right-hand side operand.
     * @return this - rhs (mod p).
     */
    [[nodiscard]] Fp sub(const Fp& rhs) const;

    /**
     * @brief Multiply two field elements.
     * @param rhs Right-hand side operand.
     * @return this * rhs (mod p).
     */
    [[nodiscard]] Fp mul(const Fp& rhs) const;

    /**
     * @brief Negate the field element.
     * @return -this (mod p).
     */
    [[nodiscard]] Fp neg() const;

    /**
     * @brief Compute the multiplicative inverse.
     * @return this^(-1) (mod p).
     * @throws std::domain_error if this is zero.
     */
    [[nodiscard]] Fp inv() const;

    /**
     * @brief Raise to a power.
     * @param exp The exponent (non-negative uint256).
     * @return this^exp (mod p).
     */
    [[nodiscard]] Fp pow(const uint256& exp) const;

    /**
     * @brief Raise to a power (uint64_t version).
     * @param exp The exponent.
     * @return this^exp (mod p).
     */
    [[nodiscard]] Fp pow(uint64_t exp) const;

    /**
     * @brief Square the field element.
     * @return this^2 (mod p).
     */
    [[nodiscard]] Fp square() const;

    // Comparison operations

    /**
     * @brief Check if this is zero.
     * @return True if this == 0.
     */
    [[nodiscard]] bool is_zero() const;

    /**
     * @brief Check if this is one.
     * @return True if this == 1.
     */
    [[nodiscard]] bool is_one() const;

    /**
     * @brief Equality comparison.
     */
    [[nodiscard]] bool operator==(const Fp& rhs) const;

    /**
     * @brief Three-way comparison (for ordering, not constant-time).
     */
    [[nodiscard]] std::strong_ordering operator<=>(const Fp& rhs) const;

    // Serialization

    /**
     * @brief Serialize to a byte array (little-endian).
     * @return 32-byte array containing the serialized value.
     */
    [[nodiscard]] std::array<uint8_t, BYTES> to_bytes() const;

    /**
     * @brief Serialize to a byte array (little-endian).
     * @param out Output span (must be at least 32 bytes).
     */
    void to_bytes(std::span<uint8_t, BYTES> out) const;

    /**
     * @brief Deserialize from a byte array (little-endian).
     * @param bytes Input byte array.
     * @return The deserialized field element.
     */
    [[nodiscard]] static Fp from_bytes(std::span<const uint8_t> bytes);

    /**
     * @brief Get the underlying uint256 value (const reference).
     * @return Reference to the internal uint256.
     */
    [[nodiscard]] const uint256& value() const { return value_; }

    /**
     * @brief Convert to a hexadecimal string.
     * @return Hexadecimal representation with 0x prefix.
     */
    [[nodiscard]] std::string to_hex() const;

    /**
     * @brief Convert to a decimal string.
     * @return Decimal representation.
     */
    [[nodiscard]] std::string to_string() const;

    // Operator overloads for convenience

    Fp operator+(const Fp& rhs) const { return add(rhs); }
    Fp operator-(const Fp& rhs) const { return sub(rhs); }
    Fp operator*(const Fp& rhs) const { return mul(rhs); }
    Fp operator-() const { return neg(); }

    Fp& operator+=(const Fp& rhs) {
        *this = add(rhs);
        return *this;
    }
    Fp& operator-=(const Fp& rhs) {
        *this = sub(rhs);
        return *this;
    }
    Fp& operator*=(const Fp& rhs) {
        *this = mul(rhs);
        return *this;
    }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Fp& fp);

private:
    uint256 value_;

    /**
     * @brief Reduce the value modulo p (internal helper).
     */
    void reduce();
};

/**
 * @brief Compute the modular inverse using Fermat's little theorem.
 * @param a The value to invert (as uint256).
 * @param m The modulus (unused, always uses p = 2^255 - 19).
 * @return a^(-1) mod p.
 */
[[nodiscard]] uint256 mod_inverse(const uint256& a, const uint256& m);

}  // namespace rescue
