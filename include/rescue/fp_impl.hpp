#pragma once

/**
 * @file fp_impl.hpp
 * @brief Optimized field arithmetic for F_p where p = 2^255 - 19.
 *
 * This file provides highly optimized field operations that exploit the
 * special structure of the Curve25519 prime. Key optimizations:
 *
 * 1. Fast reduction: 2^255 ≡ 19 (mod p), so 2^256 ≡ 38 (mod p)
 * 2. Single conditional subtraction for addition
 * 3. Optimized squaring with symmetry exploitation
 * 4. Inversion via Fermat's little theorem with optimized addition chain
 */

#include <rescue/uint256.hpp>

#include <cstdint>

namespace rescue::fp {

// ============================================================================
// The prime modulus p = 2^255 - 19
// ============================================================================

/**
 * @brief The field prime p = 2^255 - 19.
 *
 * In hex: 0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed
 * In limbs (little-endian): {0xffffffffffffffed, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff}
 */
inline constexpr uint256 P{
    0xffffffffffffffedULL,  // limb 0
    0xffffffffffffffffULL,  // limb 1
    0xffffffffffffffffULL,  // limb 2
    0x7fffffffffffffffULL   // limb 3
};

/**
 * @brief 2 * p (used in some reduction paths).
 */
inline constexpr uint256 P2{
    0xffffffffffffffdaULL,  // 2 * 0xffffffffffffffed = overflow, carry
    0xffffffffffffffffULL,
    0xffffffffffffffffULL,
    0xffffffffffffffffULL   // 2 * 0x7fffffffffffffff + carry = 0xffffffffffffffff
};

/**
 * @brief p - 2 (exponent for Fermat inversion).
 */
inline constexpr uint256 P_MINUS_2{
    0xffffffffffffffebULL,
    0xffffffffffffffffULL,
    0xffffffffffffffffULL,
    0x7fffffffffffffffULL
};

// ============================================================================
// Core field operations
// ============================================================================

/**
 * @brief Reduce a uint256 value modulo p.
 *
 * Assumes input is in [0, 2p). Uses constant-time conditional subtraction.
 */
[[nodiscard]] inline constexpr uint256 reduce_once(const uint256& x) noexcept {
    // Compute x - p
    auto [diff, borrow] = uint256::sub_with_borrow(x, P);

    // If no borrow, x >= p, return diff; otherwise return x
    // Constant-time select: mask = borrow ? 0xFF...FF : 0
    uint64_t mask = static_cast<uint64_t>(borrow) - 1;  // borrow=1 -> mask=0, borrow=0 -> mask=~0

    // Select: result = borrow ? x : diff = x ^ ((x ^ diff) & ~mask)
    // When mask=0 (borrow=1), result = x
    // When mask=~0 (borrow=0), result = diff
    return uint256{
        (x.limb(0) & ~mask) | (diff.limb(0) & mask),
        (x.limb(1) & ~mask) | (diff.limb(1) & mask),
        (x.limb(2) & ~mask) | (diff.limb(2) & mask),
        (x.limb(3) & ~mask) | (diff.limb(3) & mask)
    };
}

/**
 * @brief Fully reduce a value to [0, p).
 *
 * Handles any input up to 2^256 - 1.
 */
[[nodiscard]] inline uint256 reduce_full(const uint256& x) noexcept {
    // For values >= 2p, we need multiple subtractions
    // But for our use cases (field elements), values are always < 2p after operations
    uint256 result = x;
    result = reduce_once(result);
    result = reduce_once(result);  // Handle edge cases
    return result;
}

/**
 * @brief Field addition: (a + b) mod p.
 *
 * Assumes a, b are in [0, p). Result is in [0, p).
 */
[[nodiscard]] inline constexpr uint256 add(const uint256& a, const uint256& b) noexcept {
    // a + b is in [0, 2p - 2], so one conditional subtraction suffices
    auto [sum, carry] = uint256::add_with_carry(a, b);

    // If carry or sum >= p, subtract p
    // Compute sum - p
    auto [diff, borrow] = uint256::sub_with_borrow(sum, P);

    // If carry is set, we definitely need to subtract (result wrapped into 256 bits + p more)
    // If no carry and no borrow in subtraction, sum >= p, use diff
    // If no carry and borrow, sum < p, use sum
    bool use_diff = carry || !borrow;
    uint64_t mask = static_cast<uint64_t>(use_diff) - 1;  // use_diff ? ~0 : 0
    mask = ~mask;  // use_diff ? 0xFFFF... : 0

    return uint256{
        (sum.limb(0) & ~mask) | (diff.limb(0) & mask),
        (sum.limb(1) & ~mask) | (diff.limb(1) & mask),
        (sum.limb(2) & ~mask) | (diff.limb(2) & mask),
        (sum.limb(3) & ~mask) | (diff.limb(3) & mask)
    };
}

/**
 * @brief Field subtraction: (a - b) mod p.
 *
 * Assumes a, b are in [0, p). Result is in [0, p).
 */
[[nodiscard]] inline constexpr uint256 sub(const uint256& a, const uint256& b) noexcept {
    auto [diff, borrow] = uint256::sub_with_borrow(a, b);

    // If borrow, add p back
    // Compute diff + p
    auto [sum, _] = uint256::add_with_carry(diff, P);

    // If borrow, use sum; otherwise use diff
    uint64_t mask = static_cast<uint64_t>(borrow) - 1;  // borrow ? 0 : ~0
    mask = ~mask;  // borrow ? ~0 : 0

    return uint256{
        (diff.limb(0) & ~mask) | (sum.limb(0) & mask),
        (diff.limb(1) & ~mask) | (sum.limb(1) & mask),
        (diff.limb(2) & ~mask) | (sum.limb(2) & mask),
        (diff.limb(3) & ~mask) | (sum.limb(3) & mask)
    };
}

/**
 * @brief Field negation: (-a) mod p.
 */
[[nodiscard]] inline constexpr uint256 neg(const uint256& a) noexcept {
    // -a = p - a (if a != 0)
    // For a = 0, return 0
    auto [diff, borrow] = uint256::sub_with_borrow(P, a);

    // If a was 0, diff = p, and we should return 0
    // Check if a is zero
    bool a_is_zero = a.is_zero();
    uint64_t mask = static_cast<uint64_t>(a_is_zero) - 1;  // a_is_zero ? 0 : ~0

    return uint256{
        diff.limb(0) & mask,
        diff.limb(1) & mask,
        diff.limb(2) & mask,
        diff.limb(3) & mask
    };
}

/**
 * @brief Reduce a 512-bit product modulo p using the fast reduction.
 *
 * Since 2^256 ≡ 38 (mod p), we have:
 *   (high * 2^256 + low) ≡ (high * 38 + low) (mod p)
 *
 * This may still be >= p, so we need up to two conditional subtractions.
 *
 * This implementation is constant-time - all branches are removed and replaced
 * with constant-time conditional operations.
 */
[[nodiscard]] inline uint256 reduce_512(const uint512& x) noexcept {
    // Split into high and low 256-bit parts
    uint256 low = x.low();
    uint256 high = x.high();

#if defined(__SIZEOF_INT128__)
    using u128 = unsigned __int128;

    // Multiply high by 38 and add to low
    // high * 38 produces at most 256 + 6 = 262 bits, so we need to handle overflow
    u128 carry = 0;
    uint256 result;

    // Compute low + high * 38
    for (size_t i = 0; i < uint256::LIMBS; ++i) {
        u128 prod = static_cast<u128>(high.limb(i)) * 38;
        prod += low.limb(i);
        prod += carry;
        result.limb(i) = static_cast<uint64_t>(prod);
        carry = prod >> 64;
    }

    // Constant-time handling of remaining carry
    // carry * 38 fits in 64 bits (carry is at most ~6 bits)
    uint64_t extra = static_cast<uint64_t>(carry) * 38;
    auto [r, c] = uint256::add_with_carry(result, uint256{extra});
    result = r;

    // Constant-time: add 38 if there was overflow from the previous add
    // Use mask to conditionally add 38
    uint64_t overflow_mask = static_cast<uint64_t>(c) * 0xFFFFFFFFFFFFFFFFULL;
    uint256 add_val{38 & overflow_mask, 0, 0, 0};
    auto [r2, c2] = uint256::add_with_carry(result, add_val);
    result = r2;
    // c2 can't happen in practice, but handle it for correctness
    uint64_t overflow_mask2 = static_cast<uint64_t>(c2) * 0xFFFFFFFFFFFFFFFFULL;
    result.limb(0) += 38 & overflow_mask2;

    // Final reduction to [0, p)
    result = reduce_once(result);
    result = reduce_once(result);

    return result;
#else
    // Portable version (constant-time)
    // Multiply high by 38 using schoolbook method
    uint64_t carry = 0;
    uint256 high_times_38;

    for (size_t i = 0; i < uint256::LIMBS; ++i) {
        // Split into 32-bit halves for multiplication
        uint64_t lo32 = high.limb(i) & 0xFFFFFFFF;
        uint64_t hi32 = high.limb(i) >> 32;

        uint64_t prod_lo = lo32 * 38 + carry;
        uint64_t prod_hi = hi32 * 38 + (prod_lo >> 32);
        carry = prod_hi >> 32;
        high_times_38.limb(i) = (prod_lo & 0xFFFFFFFF) | ((prod_hi & 0xFFFFFFFF) << 32);
    }

    // Add low + high * 38
    auto [result, c1] = uint256::add_with_carry(low, high_times_38);

    // Constant-time handling of overflow
    uint64_t total_carry = carry + (c1 ? 1 : 0);
    uint64_t extra = total_carry * 38;
    auto [r, c2] = uint256::add_with_carry(result, uint256{extra});
    result = r;

    // Handle potential second overflow (constant-time)
    uint64_t overflow_mask = static_cast<uint64_t>(c2) * 0xFFFFFFFFFFFFFFFFULL;
    result.limb(0) += 38 & overflow_mask;

    // Final reduction
    result = reduce_once(result);
    result = reduce_once(result);

    return result;
#endif
}

/**
 * @brief Field multiplication: (a * b) mod p.
 */
[[nodiscard]] inline uint256 mul(const uint256& a, const uint256& b) noexcept {
    uint512 wide = mul_wide(a, b);
    return reduce_512(wide);
}

/**
 * @brief Field squaring: a^2 mod p.
 *
 * Uses optimized squaring that exploits the symmetry of the product.
 */
[[nodiscard]] inline uint256 sqr(const uint256& a) noexcept {
    uint512 wide = sqr_wide(a);
    return reduce_512(wide);
}

/**
 * @brief Optimized a^5 mod p computation.
 *
 * Since alpha=5 is fixed for the Rescue S-box, we can compute a^5 using
 * exactly 2 squarings + 1 multiply, which is much faster than general exponentiation.
 *
 * a^5 = a^4 * a = (a^2)^2 * a
 */
[[nodiscard]] inline uint256 pow5(const uint256& a) noexcept {
    uint256 a2 = sqr(a);      // a^2
    uint256 a4 = sqr(a2);     // a^4
    return mul(a4, a);        // a^5
}

// ============================================================================
// Constant-time helper functions (needed before pow())
// ============================================================================

/**
 * @brief Constant-time selection: returns a if cond is true, b otherwise.
 *
 * This function is defined early because it's needed by pow().
 */
[[nodiscard]] inline constexpr uint256 ct_select(bool cond, const uint256& a, const uint256& b) noexcept {
    uint64_t mask = static_cast<uint64_t>(cond) - 1;  // cond ? 0 : ~0
    mask = ~mask;  // cond ? ~0 : 0

    return uint256{
        (b.limb(0) & ~mask) | (a.limb(0) & mask),
        (b.limb(1) & ~mask) | (a.limb(1) & mask),
        (b.limb(2) & ~mask) | (a.limb(2) & mask),
        (b.limb(3) & ~mask) | (a.limb(3) & mask)
    };
}

/**
 * @brief Field inversion: a^(-1) mod p using Fermat's little theorem.
 *
 * Computes a^(p-2) mod p using an optimized addition chain.
 *
 * For p = 2^255 - 19:
 * p - 2 = 2^255 - 21
 *
 * We use the standard addition chain that builds up powers of 2^n - 1.
 */
[[nodiscard]] inline uint256 inv(const uint256& a) noexcept {
    // Build up a^(2^n - 1) for increasing n

    // a^(2^2 - 1) = a^3
    uint256 t0 = mul(sqr(a), a);

    // a^(2^4 - 1) = a^15
    uint256 t1 = sqr(sqr(t0));      // a^12
    t1 = mul(t1, t0);               // a^15

    // a^(2^5 - 1) = a^31
    uint256 t2 = sqr(t1);           // a^30
    t2 = mul(t2, a);                // a^31

    // a^(2^10 - 1)
    uint256 t3 = t2;
    for (int i = 0; i < 5; ++i) t3 = sqr(t3);  // a^(31 * 32) = a^992
    t3 = mul(t3, t2);               // a^1023 = a^(2^10 - 1)

    // a^(2^20 - 1)
    uint256 t4 = t3;
    for (int i = 0; i < 10; ++i) t4 = sqr(t4);
    t4 = mul(t4, t3);               // a^(2^20 - 1)

    // a^(2^40 - 1)
    uint256 t5 = t4;
    for (int i = 0; i < 20; ++i) t5 = sqr(t5);
    t5 = mul(t5, t4);               // a^(2^40 - 1)

    // a^(2^50 - 1)
    uint256 t6 = t5;
    for (int i = 0; i < 10; ++i) t6 = sqr(t6);
    t6 = mul(t6, t3);               // a^(2^50 - 1)

    // a^(2^100 - 1)
    uint256 t7 = t6;
    for (int i = 0; i < 50; ++i) t7 = sqr(t7);
    t7 = mul(t7, t6);               // a^(2^100 - 1)

    // a^(2^200 - 1)
    uint256 t8 = t7;
    for (int i = 0; i < 100; ++i) t8 = sqr(t8);
    t8 = mul(t8, t7);               // a^(2^200 - 1)

    // a^(2^250 - 1)
    uint256 t9 = t8;
    for (int i = 0; i < 50; ++i) t9 = sqr(t9);
    t9 = mul(t9, t6);               // a^(2^250 - 1)

    // a^(2^255 - 32)
    uint256 t10 = t9;
    for (int i = 0; i < 5; ++i) t10 = sqr(t10);  // a^((2^250 - 1) * 32) = a^(2^255 - 32)

    // Now we need a^(2^255 - 21) = a^(2^255 - 32) * a^11
    // Compute a^11 = a^8 * a^2 * a = a^8 * a^3
    uint256 a2 = sqr(a);
    uint256 a3 = mul(a2, a);
    uint256 a8 = sqr(sqr(a2));
    uint256 a11 = mul(a8, a3);

    // Final result: a^(2^255 - 32) * a^11 = a^(2^255 - 21)
    return mul(t10, a11);
}

/**
 * @brief Constant-time field exponentiation: a^exp mod p using Montgomery ladder.
 *
 * This implementation always performs the same sequence of operations regardless
 * of the exponent bits, preventing timing side-channel attacks.
 *
 * Uses the Montgomery ladder technique: for each bit of the exponent (from MSB to LSB),
 * we always compute both a squaring and a multiplication, then select which result
 * goes where based on the bit value.
 */
[[nodiscard]] inline uint256 pow(const uint256& base, const uint256& exp) noexcept {
    uint256 r0 = uint256::one();  // Accumulates base^(bits seen so far)
    uint256 r1 = base;            // Maintains r0 * base

    // Process all 255 bits (the maximum for our field)
    // Start from bit 254 (MSB for values < p)
    for (int i = 254; i >= 0; --i) {
        bool bit = exp.bit(static_cast<size_t>(i));

        // Always compute both branches
        uint256 r0r1 = mul(r0, r1);   // r0 * r1
        uint256 r0_sqr = sqr(r0);     // r0^2
        uint256 r1_sqr = sqr(r1);     // r1^2

        // Constant-time selection based on bit
        // If bit=0: r0 = r0^2, r1 = r0*r1
        // If bit=1: r0 = r0*r1, r1 = r1^2
        r0 = ct_select(bit, r0r1, r0_sqr);
        r1 = ct_select(bit, r1_sqr, r0r1);
    }

    return r0;
}

/**
 * @brief Constant-time field exponentiation with 64-bit exponent.
 *
 * Uses Montgomery ladder for constant-time execution.
 */
[[nodiscard]] inline uint256 pow(const uint256& base, uint64_t exp) noexcept {
    uint256 r0 = uint256::one();
    uint256 r1 = base;

    // Process all 64 bits
    for (int i = 63; i >= 0; --i) {
        bool bit = (exp >> i) & 1;

        uint256 r0r1 = mul(r0, r1);
        uint256 r0_sqr = sqr(r0);
        uint256 r1_sqr = sqr(r1);

        r0 = ct_select(bit, r0r1, r0_sqr);
        r1 = ct_select(bit, r1_sqr, r0r1);
    }

    return r0;
}

// ============================================================================
// Helper functions
// ============================================================================

/**
 * @brief Check if a value is less than p.
 */
[[nodiscard]] inline constexpr bool is_valid_field_element(const uint256& x) noexcept {
    return x < P;
}

/**
 * @brief Constant-time equality check.
 */
[[nodiscard]] inline constexpr bool ct_eq(const uint256& a, const uint256& b) noexcept {
    uint64_t diff = 0;
    diff |= a.limb(0) ^ b.limb(0);
    diff |= a.limb(1) ^ b.limb(1);
    diff |= a.limb(2) ^ b.limb(2);
    diff |= a.limb(3) ^ b.limb(3);
    return diff == 0;
}

// Note: ct_select is defined earlier (before pow) as it's needed there

/**
 * @brief Constant-time less-than comparison: returns true if a < b.
 *
 * This is constant-time because it uses subtraction with borrow,
 * which executes in constant time regardless of input values.
 */
[[nodiscard]] inline constexpr bool ct_less_than(const uint256& a, const uint256& b) noexcept {
    auto [diff, borrow] = uint256::sub_with_borrow(a, b);
    (void)diff;  // Unused, we only care about borrow
    return borrow;
}

/**
 * @brief Constant-time less-than-or-equal comparison: returns true if a <= b.
 */
[[nodiscard]] inline constexpr bool ct_less_equal(const uint256& a, const uint256& b) noexcept {
    return !ct_less_than(b, a);
}

/**
 * @brief Constant-time greater-than comparison: returns true if a > b.
 */
[[nodiscard]] inline constexpr bool ct_greater_than(const uint256& a, const uint256& b) noexcept {
    return ct_less_than(b, a);
}

}  // namespace rescue::fp
