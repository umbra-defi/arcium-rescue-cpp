#pragma once

/**
 * @file uint256.hpp
 * @brief High-performance 256-bit unsigned integer type.
 *
 * This implementation is optimized for the Curve25519 base field (p = 2^255 - 19).
 * It uses 4 x 64-bit limbs in little-endian order and leverages __uint128_t
 * for efficient multiplication on supported compilers.
 */

#include <array>
#include <compare>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

namespace rescue {

/**
 * @brief 256-bit unsigned integer stored as 4 x 64-bit limbs (little-endian).
 *
 * Limb layout: value = limbs[0] + limbs[1]*2^64 + limbs[2]*2^128 + limbs[3]*2^192
 */
class uint256 {
public:
    /// Number of 64-bit limbs
    static constexpr size_t LIMBS = 4;

    /// Number of bytes in the representation
    static constexpr size_t BYTES = 32;

    /// Number of bits
    static constexpr size_t BITS = 256;

    /// Storage type
    using limb_t = uint64_t;
    using storage_t = std::array<limb_t, LIMBS>;

    // ========================================================================
    // Constructors
    // ========================================================================

    /**
     * @brief Default constructor - initializes to zero.
     */
    constexpr uint256() noexcept : limbs_{0, 0, 0, 0} {}

    /**
     * @brief Construct from a single 64-bit value.
     */
    constexpr explicit uint256(uint64_t value) noexcept : limbs_{value, 0, 0, 0} {}

    /**
     * @brief Construct from four 64-bit limbs (little-endian order).
     */
    constexpr uint256(uint64_t l0, uint64_t l1, uint64_t l2, uint64_t l3) noexcept
        : limbs_{l0, l1, l2, l3} {}

    /**
     * @brief Construct from a limb array.
     */
    constexpr explicit uint256(const storage_t& limbs) noexcept : limbs_(limbs) {}

    /**
     * @brief Construct from a byte array (little-endian).
     */
    explicit uint256(std::span<const uint8_t> bytes) noexcept {
        from_bytes_le(bytes);
    }

    /**
     * @brief Construct from a hexadecimal string.
     * @param hex Hex string with optional "0x" prefix.
     */
    explicit uint256(std::string_view hex);

    // Default copy/move
    constexpr uint256(const uint256&) noexcept = default;
    constexpr uint256(uint256&&) noexcept = default;
    constexpr uint256& operator=(const uint256&) noexcept = default;
    constexpr uint256& operator=(uint256&&) noexcept = default;
    ~uint256() = default;

    // ========================================================================
    // Static constants
    // ========================================================================

    /**
     * @brief Zero constant.
     */
    [[nodiscard]] static constexpr uint256 zero() noexcept { return uint256{}; }

    /**
     * @brief One constant.
     */
    [[nodiscard]] static constexpr uint256 one() noexcept { return uint256{1}; }

    /**
     * @brief Maximum value (all bits set).
     */
    [[nodiscard]] static constexpr uint256 max() noexcept {
        return uint256{~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}};
    }

    // ========================================================================
    // Accessors
    // ========================================================================

    /**
     * @brief Get limb by index.
     */
    [[nodiscard]] constexpr uint64_t limb(size_t i) const noexcept { return limbs_[i]; }

    /**
     * @brief Get mutable reference to limb.
     */
    [[nodiscard]] constexpr uint64_t& limb(size_t i) noexcept { return limbs_[i]; }

    /**
     * @brief Get the underlying limb array.
     */
    [[nodiscard]] constexpr const storage_t& limbs() const noexcept { return limbs_; }

    /**
     * @brief Get mutable reference to limb array.
     */
    [[nodiscard]] constexpr storage_t& limbs() noexcept { return limbs_; }

    /**
     * @brief Check if value is zero.
     */
    [[nodiscard]] constexpr bool is_zero() const noexcept {
        return (limbs_[0] | limbs_[1] | limbs_[2] | limbs_[3]) == 0;
    }

    /**
     * @brief Check if value is one.
     */
    [[nodiscard]] constexpr bool is_one() const noexcept {
        return limbs_[0] == 1 && (limbs_[1] | limbs_[2] | limbs_[3]) == 0;
    }

    /**
     * @brief Get a specific bit.
     * @param pos Bit position (0 = LSB).
     */
    [[nodiscard]] constexpr bool bit(size_t pos) const noexcept {
        size_t limb_idx = pos / 64;
        size_t bit_idx = pos % 64;
        if (limb_idx >= LIMBS) return false;
        return (limbs_[limb_idx] >> bit_idx) & 1;
    }

    /**
     * @brief Set a specific bit.
     */
    constexpr void set_bit(size_t pos) noexcept {
        size_t limb_idx = pos / 64;
        size_t bit_idx = pos % 64;
        if (limb_idx < LIMBS) {
            limbs_[limb_idx] |= (uint64_t{1} << bit_idx);
        }
    }

    /**
     * @brief Clear a specific bit.
     */
    constexpr void clear_bit(size_t pos) noexcept {
        size_t limb_idx = pos / 64;
        size_t bit_idx = pos % 64;
        if (limb_idx < LIMBS) {
            limbs_[limb_idx] &= ~(uint64_t{1} << bit_idx);
        }
    }

    /**
     * @brief Get the number of significant bits (position of highest set bit + 1).
     */
    [[nodiscard]] constexpr size_t bit_length() const noexcept {
        for (int i = LIMBS - 1; i >= 0; --i) {
            if (limbs_[i] != 0) {
                // Count leading zeros and compute bit position
                return static_cast<size_t>(i) * 64 + (64 - clz64(limbs_[i]));
            }
        }
        return 0;
    }

    // ========================================================================
    // Comparison operators
    // ========================================================================

    [[nodiscard]] constexpr bool operator==(const uint256& rhs) const noexcept {
        return limbs_[0] == rhs.limbs_[0] && limbs_[1] == rhs.limbs_[1] &&
               limbs_[2] == rhs.limbs_[2] && limbs_[3] == rhs.limbs_[3];
    }

    [[nodiscard]] constexpr bool operator!=(const uint256& rhs) const noexcept {
        return !(*this == rhs);
    }

    [[nodiscard]] constexpr bool operator<(const uint256& rhs) const noexcept {
        for (int i = LIMBS - 1; i >= 0; --i) {
            if (limbs_[i] < rhs.limbs_[i]) return true;
            if (limbs_[i] > rhs.limbs_[i]) return false;
        }
        return false;
    }

    [[nodiscard]] constexpr bool operator<=(const uint256& rhs) const noexcept {
        return !(rhs < *this);
    }

    [[nodiscard]] constexpr bool operator>(const uint256& rhs) const noexcept {
        return rhs < *this;
    }

    [[nodiscard]] constexpr bool operator>=(const uint256& rhs) const noexcept {
        return !(*this < rhs);
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(const uint256& rhs) const noexcept {
        for (int i = LIMBS - 1; i >= 0; --i) {
            if (limbs_[i] < rhs.limbs_[i]) return std::strong_ordering::less;
            if (limbs_[i] > rhs.limbs_[i]) return std::strong_ordering::greater;
        }
        return std::strong_ordering::equal;
    }

    // ========================================================================
    // Basic arithmetic (non-modular)
    // ========================================================================

    /**
     * @brief Add with carry, returning the result and carry out.
     * @return {result, carry_out}
     */
    [[nodiscard]] static constexpr std::pair<uint256, bool> add_with_carry(
        const uint256& a, const uint256& b) noexcept {
        uint256 result;
        uint64_t carry = 0;

        for (size_t i = 0; i < LIMBS; ++i) {
            uint64_t sum = a.limbs_[i] + carry;
            carry = (sum < a.limbs_[i]) ? 1 : 0;
            uint64_t sum2 = sum + b.limbs_[i];
            carry += (sum2 < sum) ? 1 : 0;
            result.limbs_[i] = sum2;
        }

        return {result, carry != 0};
    }

    /**
     * @brief Subtract with borrow, returning the result and borrow out.
     * @return {result, borrow_out}
     */
    [[nodiscard]] static constexpr std::pair<uint256, bool> sub_with_borrow(
        const uint256& a, const uint256& b) noexcept {
        uint256 result;
        uint64_t borrow = 0;

        for (size_t i = 0; i < LIMBS; ++i) {
            uint64_t diff = a.limbs_[i] - borrow;
            borrow = (diff > a.limbs_[i]) ? 1 : 0;
            uint64_t diff2 = diff - b.limbs_[i];
            borrow += (diff2 > diff) ? 1 : 0;
            result.limbs_[i] = diff2;
        }

        return {result, borrow != 0};
    }

    /**
     * @brief Add two uint256 values (wrapping on overflow).
     */
    [[nodiscard]] constexpr uint256 operator+(const uint256& rhs) const noexcept {
        return add_with_carry(*this, rhs).first;
    }

    /**
     * @brief Subtract two uint256 values (wrapping on underflow).
     */
    [[nodiscard]] constexpr uint256 operator-(const uint256& rhs) const noexcept {
        return sub_with_borrow(*this, rhs).first;
    }

    constexpr uint256& operator+=(const uint256& rhs) noexcept {
        *this = *this + rhs;
        return *this;
    }

    constexpr uint256& operator-=(const uint256& rhs) noexcept {
        *this = *this - rhs;
        return *this;
    }

    // ========================================================================
    // Bit operations
    // ========================================================================

    [[nodiscard]] constexpr uint256 operator&(const uint256& rhs) const noexcept {
        return uint256{limbs_[0] & rhs.limbs_[0], limbs_[1] & rhs.limbs_[1],
                       limbs_[2] & rhs.limbs_[2], limbs_[3] & rhs.limbs_[3]};
    }

    [[nodiscard]] constexpr uint256 operator|(const uint256& rhs) const noexcept {
        return uint256{limbs_[0] | rhs.limbs_[0], limbs_[1] | rhs.limbs_[1],
                       limbs_[2] | rhs.limbs_[2], limbs_[3] | rhs.limbs_[3]};
    }

    [[nodiscard]] constexpr uint256 operator^(const uint256& rhs) const noexcept {
        return uint256{limbs_[0] ^ rhs.limbs_[0], limbs_[1] ^ rhs.limbs_[1],
                       limbs_[2] ^ rhs.limbs_[2], limbs_[3] ^ rhs.limbs_[3]};
    }

    [[nodiscard]] constexpr uint256 operator~() const noexcept {
        return uint256{~limbs_[0], ~limbs_[1], ~limbs_[2], ~limbs_[3]};
    }

    constexpr uint256& operator&=(const uint256& rhs) noexcept {
        *this = *this & rhs;
        return *this;
    }

    constexpr uint256& operator|=(const uint256& rhs) noexcept {
        *this = *this | rhs;
        return *this;
    }

    constexpr uint256& operator^=(const uint256& rhs) noexcept {
        *this = *this ^ rhs;
        return *this;
    }

    /**
     * @brief Left shift by n bits.
     */
    [[nodiscard]] constexpr uint256 operator<<(size_t n) const noexcept {
        if (n >= BITS) return uint256{};
        if (n == 0) return *this;

        uint256 result{};
        size_t limb_shift = n / 64;
        size_t bit_shift = n % 64;

        if (bit_shift == 0) {
            for (size_t i = limb_shift; i < LIMBS; ++i) {
                result.limbs_[i] = limbs_[i - limb_shift];
            }
        } else {
            for (size_t i = limb_shift; i < LIMBS; ++i) {
                result.limbs_[i] = limbs_[i - limb_shift] << bit_shift;
                if (i > limb_shift) {
                    result.limbs_[i] |= limbs_[i - limb_shift - 1] >> (64 - bit_shift);
                }
            }
        }

        return result;
    }

    /**
     * @brief Right shift by n bits.
     */
    [[nodiscard]] constexpr uint256 operator>>(size_t n) const noexcept {
        if (n >= BITS) return uint256{};
        if (n == 0) return *this;

        uint256 result{};
        size_t limb_shift = n / 64;
        size_t bit_shift = n % 64;

        if (bit_shift == 0) {
            for (size_t i = 0; i < LIMBS - limb_shift; ++i) {
                result.limbs_[i] = limbs_[i + limb_shift];
            }
        } else {
            for (size_t i = 0; i < LIMBS - limb_shift; ++i) {
                result.limbs_[i] = limbs_[i + limb_shift] >> bit_shift;
                if (i + limb_shift + 1 < LIMBS) {
                    result.limbs_[i] |= limbs_[i + limb_shift + 1] << (64 - bit_shift);
                }
            }
        }

        return result;
    }

    constexpr uint256& operator<<=(size_t n) noexcept {
        *this = *this << n;
        return *this;
    }

    constexpr uint256& operator>>=(size_t n) noexcept {
        *this = *this >> n;
        return *this;
    }

    // ========================================================================
    // Serialization
    // ========================================================================

    /**
     * @brief Serialize to little-endian bytes.
     */
    [[nodiscard]] std::array<uint8_t, BYTES> to_bytes_le() const noexcept {
        std::array<uint8_t, BYTES> result;
        for (size_t i = 0; i < LIMBS; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                result[i * 8 + j] = static_cast<uint8_t>(limbs_[i] >> (j * 8));
            }
        }
        return result;
    }

    /**
     * @brief Serialize to little-endian bytes (output span version).
     */
    void to_bytes_le(std::span<uint8_t, BYTES> out) const noexcept {
        for (size_t i = 0; i < LIMBS; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                out[i * 8 + j] = static_cast<uint8_t>(limbs_[i] >> (j * 8));
            }
        }
    }

    /**
     * @brief Deserialize from little-endian bytes.
     */
    void from_bytes_le(std::span<const uint8_t> bytes) noexcept {
        limbs_ = {0, 0, 0, 0};
        size_t len = std::min(bytes.size(), BYTES);
        for (size_t i = 0; i < len; ++i) {
            limbs_[i / 8] |= static_cast<uint64_t>(bytes[i]) << ((i % 8) * 8);
        }
    }

    /**
     * @brief Create from little-endian bytes.
     */
    [[nodiscard]] static uint256 from_bytes(std::span<const uint8_t> bytes) noexcept {
        uint256 result;
        result.from_bytes_le(bytes);
        return result;
    }

    /**
     * @brief Convert to hexadecimal string (with 0x prefix).
     */
    [[nodiscard]] std::string to_hex() const;

    /**
     * @brief Convert to decimal string.
     */
    [[nodiscard]] std::string to_string() const;

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const uint256& val);

private:
    storage_t limbs_;

    /**
     * @brief Count leading zeros in a 64-bit value.
     */
    [[nodiscard]] static constexpr int clz64(uint64_t x) noexcept {
        if (x == 0) return 64;
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_clzll(x);
#else
        // Portable fallback
        int n = 0;
        if ((x & 0xFFFFFFFF00000000ULL) == 0) { n += 32; x <<= 32; }
        if ((x & 0xFFFF000000000000ULL) == 0) { n += 16; x <<= 16; }
        if ((x & 0xFF00000000000000ULL) == 0) { n += 8;  x <<= 8; }
        if ((x & 0xF000000000000000ULL) == 0) { n += 4;  x <<= 4; }
        if ((x & 0xC000000000000000ULL) == 0) { n += 2;  x <<= 2; }
        if ((x & 0x8000000000000000ULL) == 0) { n += 1; }
        return n;
#endif
    }
};

// ============================================================================
// 512-bit intermediate type for multiplication
// ============================================================================

/**
 * @brief 512-bit unsigned integer for multiplication intermediates.
 * Stored as 8 x 64-bit limbs in little-endian order.
 */
class uint512 {
public:
    static constexpr size_t LIMBS = 8;
    using limb_t = uint64_t;
    using storage_t = std::array<limb_t, LIMBS>;

    constexpr uint512() noexcept : limbs_{} {}

    constexpr explicit uint512(const storage_t& limbs) noexcept : limbs_(limbs) {}

    [[nodiscard]] constexpr uint64_t limb(size_t i) const noexcept { return limbs_[i]; }
    [[nodiscard]] constexpr uint64_t& limb(size_t i) noexcept { return limbs_[i]; }
    [[nodiscard]] constexpr const storage_t& limbs() const noexcept { return limbs_; }
    [[nodiscard]] constexpr storage_t& limbs() noexcept { return limbs_; }

    /**
     * @brief Get the low 256 bits.
     */
    [[nodiscard]] constexpr uint256 low() const noexcept {
        return uint256{limbs_[0], limbs_[1], limbs_[2], limbs_[3]};
    }

    /**
     * @brief Get the high 256 bits.
     */
    [[nodiscard]] constexpr uint256 high() const noexcept {
        return uint256{limbs_[4], limbs_[5], limbs_[6], limbs_[7]};
    }

private:
    storage_t limbs_;
};

// ============================================================================
// Wide multiplication
// ============================================================================

/**
 * @brief Multiply two uint256 values to produce a uint512 result.
 *
 * Uses schoolbook multiplication with __uint128_t for efficiency.
 */
[[nodiscard]] inline uint512 mul_wide(const uint256& a, const uint256& b) noexcept {
    uint512 result;

#if defined(__SIZEOF_INT128__)
    // Use __uint128_t for efficient 64x64 -> 128 bit multiplication
    using u128 = unsigned __int128;

    u128 carry = 0;

    // Schoolbook multiplication
    for (size_t i = 0; i < uint256::LIMBS; ++i) {
        carry = 0;
        for (size_t j = 0; j < uint256::LIMBS; ++j) {
            u128 prod = static_cast<u128>(a.limb(i)) * static_cast<u128>(b.limb(j));
            prod += result.limb(i + j);
            prod += carry;
            result.limb(i + j) = static_cast<uint64_t>(prod);
            carry = prod >> 64;
        }
        result.limb(i + uint256::LIMBS) = static_cast<uint64_t>(carry);
    }
#else
    // Portable fallback using 32-bit multiplication
    // Split each 64-bit limb into two 32-bit halves
    auto mul32 = [](uint32_t a, uint32_t b) -> uint64_t {
        return static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
    };

    std::array<uint32_t, 8> a32, b32;
    for (size_t i = 0; i < 4; ++i) {
        a32[i * 2] = static_cast<uint32_t>(a.limb(i));
        a32[i * 2 + 1] = static_cast<uint32_t>(a.limb(i) >> 32);
        b32[i * 2] = static_cast<uint32_t>(b.limb(i));
        b32[i * 2 + 1] = static_cast<uint32_t>(b.limb(i) >> 32);
    }

    std::array<uint64_t, 16> temp{};
    for (size_t i = 0; i < 8; ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < 8; ++j) {
            uint64_t prod = mul32(a32[i], b32[j]) + temp[i + j] + carry;
            temp[i + j] = prod & 0xFFFFFFFF;
            carry = prod >> 32;
        }
        temp[i + 8] += carry;
    }

    // Combine 32-bit results into 64-bit limbs
    for (size_t i = 0; i < 8; ++i) {
        result.limb(i) = temp[i * 2] | (temp[i * 2 + 1] << 32);
    }
#endif

    return result;
}

/**
 * @brief Square a uint256 value to produce a uint512 result.
 *
 * Optimized squaring that takes advantage of symmetry.
 */
[[nodiscard]] inline uint512 sqr_wide(const uint256& a) noexcept {
    uint512 result;

#if defined(__SIZEOF_INT128__)
    using u128 = unsigned __int128;

    // First, compute all the cross products (each appears twice)
    for (size_t i = 0; i < uint256::LIMBS; ++i) {
        u128 carry = 0;
        for (size_t j = i + 1; j < uint256::LIMBS; ++j) {
            u128 prod = static_cast<u128>(a.limb(i)) * static_cast<u128>(a.limb(j));
            prod += result.limb(i + j);
            prod += carry;
            result.limb(i + j) = static_cast<uint64_t>(prod);
            carry = prod >> 64;
        }
        result.limb(i + uint256::LIMBS) = static_cast<uint64_t>(carry);
    }

    // Double all cross products
    uint64_t carry = 0;
    for (size_t i = 1; i < uint512::LIMBS; ++i) {
        u128 doubled = (static_cast<u128>(result.limb(i)) << 1) + carry;
        result.limb(i) = static_cast<uint64_t>(doubled);
        carry = static_cast<uint64_t>(doubled >> 64);
    }

    // Add the square terms
    carry = 0;
    for (size_t i = 0; i < uint256::LIMBS; ++i) {
        u128 sq = static_cast<u128>(a.limb(i)) * static_cast<u128>(a.limb(i));

        u128 sum = static_cast<u128>(result.limb(2 * i)) + static_cast<uint64_t>(sq) + carry;
        result.limb(2 * i) = static_cast<uint64_t>(sum);
        carry = sum >> 64;

        sum = static_cast<u128>(result.limb(2 * i + 1)) + (sq >> 64) + carry;
        result.limb(2 * i + 1) = static_cast<uint64_t>(sum);
        carry = sum >> 64;
    }
#else
    // Fallback to regular multiplication for portability
    result = mul_wide(a, a);
#endif

    return result;
}

}  // namespace rescue
