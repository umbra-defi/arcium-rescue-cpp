#pragma once

/**
 * @file mds_precomputed.hpp
 * @brief Precomputed MDS matrices for standard Rescue configurations.
 *
 * This file contains hardcoded MDS matrices for:
 * - m=5: Cipher mode (RESCUE_CIPHER_BLOCK_SIZE)
 *
 * The Cauchy MDS matrix is defined as M[i][j] = 1/(i+j) for i,j = 1..m
 * computed over the field F_p where p = 2^255 - 19.
 *
 * Precomputing these matrices eliminates the overhead of computing modular
 * inverses during initialization.
 */

#include <rescue/uint256.hpp>

#include <array>
#include <cstddef>

namespace rescue::mds {

// ============================================================================
// MDS Matrix for m=5 (Cipher mode)
// M[i][j] = 1/(i+j) mod p, for i,j in [1,5]
// ============================================================================

/**
 * @brief Precomputed 5x5 MDS matrix for cipher mode.
 *
 * Entry (i,j) = 1/(i+j+2) mod p, since array is 0-indexed.
 */
inline constexpr std::array<std::array<uint256, 5>, 5> MDS_5x5 = {{
    // Row 0: 1/2, 1/3, 1/4, 1/5, 1/6
    {{
        uint256{0xfffffffffffffff7ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x3fffffffffffffffULL},  // 1/2
        uint256{0x5555555555555549ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x5555555555555555ULL},  // 1/3
        uint256{0xfffffffffffffff2ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x5fffffffffffffffULL},  // 1/4
        uint256{0x9999999999999996ULL, 0x9999999999999999ULL, 0x9999999999999999ULL, 0x1999999999999999ULL},  // 1/5
        uint256{0xaaaaaaaaaaaaaa9bULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x6aaaaaaaaaaaaaaaULL},  // 1/6
    }},
    // Row 1: 1/3, 1/4, 1/5, 1/6, 1/7
    {{
        uint256{0x5555555555555549ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x5555555555555555ULL},  // 1/3
        uint256{0xfffffffffffffff2ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x5fffffffffffffffULL},  // 1/4
        uint256{0x9999999999999996ULL, 0x9999999999999999ULL, 0x9999999999999999ULL, 0x1999999999999999ULL},  // 1/5
        uint256{0xaaaaaaaaaaaaaa9bULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x6aaaaaaaaaaaaaaaULL},  // 1/6
        uint256{0x249249249249248dULL, 0x9249249249249249ULL, 0x4924924924924924ULL, 0x2492492492492492ULL},  // 1/7
    }},
    // Row 2: 1/4, 1/5, 1/6, 1/7, 1/8
    {{
        uint256{0xfffffffffffffff2ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x5fffffffffffffffULL},  // 1/4
        uint256{0x9999999999999996ULL, 0x9999999999999999ULL, 0x9999999999999999ULL, 0x1999999999999999ULL},  // 1/5
        uint256{0xaaaaaaaaaaaaaa9bULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x6aaaaaaaaaaaaaaaULL},  // 1/6
        uint256{0x249249249249248dULL, 0x9249249249249249ULL, 0x4924924924924924ULL, 0x2492492492492492ULL},  // 1/7
        uint256{0xfffffffffffffff9ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x2fffffffffffffffULL},  // 1/8
    }},
    // Row 3: 1/5, 1/6, 1/7, 1/8, 1/9
    {{
        uint256{0x9999999999999996ULL, 0x9999999999999999ULL, 0x9999999999999999ULL, 0x1999999999999999ULL},  // 1/5
        uint256{0xaaaaaaaaaaaaaa9bULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x6aaaaaaaaaaaaaaaULL},  // 1/6
        uint256{0x249249249249248dULL, 0x9249249249249249ULL, 0x4924924924924924ULL, 0x2492492492492492ULL},  // 1/7
        uint256{0xfffffffffffffff9ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x2fffffffffffffffULL},  // 1/8
        uint256{0xc71c71c71c71c712ULL, 0x1c71c71c71c71c71ULL, 0x71c71c71c71c71c7ULL, 0x471c71c71c71c71cULL},  // 1/9
    }},
    // Row 4: 1/6, 1/7, 1/8, 1/9, 1/10
    {{
        uint256{0xaaaaaaaaaaaaaa9bULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x6aaaaaaaaaaaaaaaULL},  // 1/6
        uint256{0x249249249249248dULL, 0x9249249249249249ULL, 0x4924924924924924ULL, 0x2492492492492492ULL},  // 1/7
        uint256{0xfffffffffffffff9ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x2fffffffffffffffULL},  // 1/8
        uint256{0xc71c71c71c71c712ULL, 0x1c71c71c71c71c71ULL, 0x71c71c71c71c71c7ULL, 0x471c71c71c71c71cULL},  // 1/9
        uint256{0xcccccccccccccccbULL, 0xccccccccccccccccULL, 0xccccccccccccccccULL, 0x0cccccccccccccccULL},  // 1/10
    }},
}};

// ============================================================================
// MDS Matrix for m=12 (Hash mode)
// M[i][j] = 1/(i+j) mod p, for i,j in [1,12]
// ============================================================================

/**
 * @brief Precomputed 12x12 MDS matrix for hash mode.
 *
 * Entry (i,j) = 1/(i+j+2) mod p, since array is 0-indexed.
 */
inline constexpr std::array<std::array<uint256, 12>, 12> MDS_12x12 = {{
    // Row 0: 1/2, 1/3, 1/4, 1/5, 1/6, 1/7, 1/8, 1/9, 1/10, 1/11, 1/12, 1/13
    {{
        uint256{0xfffffffffffffff7ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x3fffffffffffffffULL},
        uint256{0x5555555555555549ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x5555555555555555ULL},
        uint256{0xfffffffffffffff2ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x5fffffffffffffffULL},
        uint256{0x9999999999999996ULL, 0x9999999999999999ULL, 0x9999999999999999ULL, 0x1999999999999999ULL},
        uint256{0xaaaaaaaaaaaaaa9bULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x6aaaaaaaaaaaaaaaULL},
        uint256{0x249249249249248dULL, 0x9249249249249249ULL, 0x4924924924924924ULL, 0x2492492492492492ULL},
        uint256{0xfffffffffffffff9ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x2fffffffffffffffULL},
        uint256{0xc71c71c71c71c712ULL, 0x1c71c71c71c71c71ULL, 0x71c71c71c71c71c7ULL, 0x471c71c71c71c71cULL},
        uint256{0xcccccccccccccccbULL, 0xccccccccccccccccULL, 0xccccccccccccccccULL, 0x0cccccccccccccccULL},
        uint256{0xe8ba2e8ba2e8ba26ULL, 0x2e8ba2e8ba2e8ba2ULL, 0xa2e8ba2e8ba2e8baULL, 0x3a2e8ba2e8ba2e8bULL},
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
    }},
    // Row 1: 1/3, 1/4, 1/5, 1/6, 1/7, 1/8, 1/9, 1/10, 1/11, 1/12, 1/13, 1/14
    {{
        uint256{0x5555555555555549ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x5555555555555555ULL},
        uint256{0xfffffffffffffff2ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x5fffffffffffffffULL},
        uint256{0x9999999999999996ULL, 0x9999999999999999ULL, 0x9999999999999999ULL, 0x1999999999999999ULL},
        uint256{0xaaaaaaaaaaaaaa9bULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x6aaaaaaaaaaaaaaaULL},
        uint256{0x249249249249248dULL, 0x9249249249249249ULL, 0x4924924924924924ULL, 0x2492492492492492ULL},
        uint256{0xfffffffffffffff9ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x2fffffffffffffffULL},
        uint256{0xc71c71c71c71c712ULL, 0x1c71c71c71c71c71ULL, 0x71c71c71c71c71c7ULL, 0x471c71c71c71c71cULL},
        uint256{0xcccccccccccccccbULL, 0xccccccccccccccccULL, 0xccccccccccccccccULL, 0x0cccccccccccccccULL},
        uint256{0xe8ba2e8ba2e8ba26ULL, 0x2e8ba2e8ba2e8ba2ULL, 0xa2e8ba2e8ba2e8baULL, 0x3a2e8ba2e8ba2e8bULL},
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
    }},
    // Row 2: 1/4, 1/5, 1/6, 1/7, 1/8, 1/9, 1/10, 1/11, 1/12, 1/13, 1/14, 1/15
    {{
        uint256{0xfffffffffffffff2ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x5fffffffffffffffULL},
        uint256{0x9999999999999996ULL, 0x9999999999999999ULL, 0x9999999999999999ULL, 0x1999999999999999ULL},
        uint256{0xaaaaaaaaaaaaaa9bULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x6aaaaaaaaaaaaaaaULL},
        uint256{0x249249249249248dULL, 0x9249249249249249ULL, 0x4924924924924924ULL, 0x2492492492492492ULL},
        uint256{0xfffffffffffffff9ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x2fffffffffffffffULL},
        uint256{0xc71c71c71c71c712ULL, 0x1c71c71c71c71c71ULL, 0x71c71c71c71c71c7ULL, 0x471c71c71c71c71cULL},
        uint256{0xcccccccccccccccbULL, 0xccccccccccccccccULL, 0xccccccccccccccccULL, 0x0cccccccccccccccULL},
        uint256{0xe8ba2e8ba2e8ba26ULL, 0x2e8ba2e8ba2e8ba2ULL, 0xa2e8ba2e8ba2e8baULL, 0x3a2e8ba2e8ba2e8bULL},
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
        uint256{0xddddddddddddddd0ULL, 0xddddddddddddddddULL, 0xddddddddddddddddULL, 0x5dddddddddddddddULL},
    }},
    // Row 3: 1/5, 1/6, 1/7, 1/8, 1/9, 1/10, 1/11, 1/12, 1/13, 1/14, 1/15, 1/16
    {{
        uint256{0x9999999999999996ULL, 0x9999999999999999ULL, 0x9999999999999999ULL, 0x1999999999999999ULL},
        uint256{0xaaaaaaaaaaaaaa9bULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x6aaaaaaaaaaaaaaaULL},
        uint256{0x249249249249248dULL, 0x9249249249249249ULL, 0x4924924924924924ULL, 0x2492492492492492ULL},
        uint256{0xfffffffffffffff9ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x2fffffffffffffffULL},
        uint256{0xc71c71c71c71c712ULL, 0x1c71c71c71c71c71ULL, 0x71c71c71c71c71c7ULL, 0x471c71c71c71c71cULL},
        uint256{0xcccccccccccccccbULL, 0xccccccccccccccccULL, 0xccccccccccccccccULL, 0x0cccccccccccccccULL},
        uint256{0xe8ba2e8ba2e8ba26ULL, 0x2e8ba2e8ba2e8ba2ULL, 0xa2e8ba2e8ba2e8baULL, 0x3a2e8ba2e8ba2e8bULL},
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
        uint256{0xddddddddddddddd0ULL, 0xddddddddddddddddULL, 0xddddddddddddddddULL, 0x5dddddddddddddddULL},
        uint256{0xfffffffffffffff3ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x57ffffffffffffffULL},
    }},
    // Row 4: 1/6, 1/7, 1/8, 1/9, 1/10, 1/11, 1/12, 1/13, 1/14, 1/15, 1/16, 1/17
    {{
        uint256{0xaaaaaaaaaaaaaa9bULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x6aaaaaaaaaaaaaaaULL},
        uint256{0x249249249249248dULL, 0x9249249249249249ULL, 0x4924924924924924ULL, 0x2492492492492492ULL},
        uint256{0xfffffffffffffff9ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x2fffffffffffffffULL},
        uint256{0xc71c71c71c71c712ULL, 0x1c71c71c71c71c71ULL, 0x71c71c71c71c71c7ULL, 0x471c71c71c71c71cULL},
        uint256{0xcccccccccccccccbULL, 0xccccccccccccccccULL, 0xccccccccccccccccULL, 0x0cccccccccccccccULL},
        uint256{0xe8ba2e8ba2e8ba26ULL, 0x2e8ba2e8ba2e8ba2ULL, 0xa2e8ba2e8ba2e8baULL, 0x3a2e8ba2e8ba2e8bULL},
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
        uint256{0xddddddddddddddd0ULL, 0xddddddddddddddddULL, 0xddddddddddddddddULL, 0x5dddddddddddddddULL},
        uint256{0xfffffffffffffff3ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x57ffffffffffffffULL},
        uint256{0x5a5a5a5a5a5a5a4dULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL},
    }},
    // Row 5: 1/7, 1/8, 1/9, 1/10, 1/11, 1/12, 1/13, 1/14, 1/15, 1/16, 1/17, 1/18
    {{
        uint256{0x249249249249248dULL, 0x9249249249249249ULL, 0x4924924924924924ULL, 0x2492492492492492ULL},
        uint256{0xfffffffffffffff9ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x2fffffffffffffffULL},
        uint256{0xc71c71c71c71c712ULL, 0x1c71c71c71c71c71ULL, 0x71c71c71c71c71c7ULL, 0x471c71c71c71c71cULL},
        uint256{0xcccccccccccccccbULL, 0xccccccccccccccccULL, 0xccccccccccccccccULL, 0x0cccccccccccccccULL},
        uint256{0xe8ba2e8ba2e8ba26ULL, 0x2e8ba2e8ba2e8ba2ULL, 0xa2e8ba2e8ba2e8baULL, 0x3a2e8ba2e8ba2e8bULL},
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
        uint256{0xddddddddddddddd0ULL, 0xddddddddddddddddULL, 0xddddddddddddddddULL, 0x5dddddddddddddddULL},
        uint256{0xfffffffffffffff3ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x57ffffffffffffffULL},
        uint256{0x5a5a5a5a5a5a5a4dULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL},
        uint256{0xe38e38e38e38e389ULL, 0x8e38e38e38e38e38ULL, 0x38e38e38e38e38e3ULL, 0x238e38e38e38e38eULL},
    }},
    // Row 6: 1/8, 1/9, 1/10, 1/11, 1/12, 1/13, 1/14, 1/15, 1/16, 1/17, 1/18, 1/19
    {{
        uint256{0xfffffffffffffff9ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x2fffffffffffffffULL},
        uint256{0xc71c71c71c71c712ULL, 0x1c71c71c71c71c71ULL, 0x71c71c71c71c71c7ULL, 0x471c71c71c71c71cULL},
        uint256{0xcccccccccccccccbULL, 0xccccccccccccccccULL, 0xccccccccccccccccULL, 0x0cccccccccccccccULL},
        uint256{0xe8ba2e8ba2e8ba26ULL, 0x2e8ba2e8ba2e8ba2ULL, 0xa2e8ba2e8ba2e8baULL, 0x3a2e8ba2e8ba2e8bULL},
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
        uint256{0xddddddddddddddd0ULL, 0xddddddddddddddddULL, 0xddddddddddddddddULL, 0x5dddddddddddddddULL},
        uint256{0xfffffffffffffff3ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x57ffffffffffffffULL},
        uint256{0x5a5a5a5a5a5a5a4dULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL},
        uint256{0xe38e38e38e38e389ULL, 0x8e38e38e38e38e38ULL, 0x38e38e38e38e38e3ULL, 0x238e38e38e38e38eULL},
        uint256{0x86bca1af286bca14ULL, 0xbca1af286bca1af2ULL, 0xa1af286bca1af286ULL, 0x2f286bca1af286bcULL},
    }},
    // Row 7: 1/9, 1/10, 1/11, 1/12, 1/13, 1/14, 1/15, 1/16, 1/17, 1/18, 1/19, 1/20
    {{
        uint256{0xc71c71c71c71c712ULL, 0x1c71c71c71c71c71ULL, 0x71c71c71c71c71c7ULL, 0x471c71c71c71c71cULL},
        uint256{0xcccccccccccccccbULL, 0xccccccccccccccccULL, 0xccccccccccccccccULL, 0x0cccccccccccccccULL},
        uint256{0xe8ba2e8ba2e8ba26ULL, 0x2e8ba2e8ba2e8ba2ULL, 0xa2e8ba2e8ba2e8baULL, 0x3a2e8ba2e8ba2e8bULL},
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
        uint256{0xddddddddddddddd0ULL, 0xddddddddddddddddULL, 0xddddddddddddddddULL, 0x5dddddddddddddddULL},
        uint256{0xfffffffffffffff3ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x57ffffffffffffffULL},
        uint256{0x5a5a5a5a5a5a5a4dULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL},
        uint256{0xe38e38e38e38e389ULL, 0x8e38e38e38e38e38ULL, 0x38e38e38e38e38e3ULL, 0x238e38e38e38e38eULL},
        uint256{0x86bca1af286bca14ULL, 0xbca1af286bca1af2ULL, 0xa1af286bca1af286ULL, 0x2f286bca1af286bcULL},
        uint256{0x666666666666665cULL, 0x6666666666666666ULL, 0x6666666666666666ULL, 0x4666666666666666ULL},
    }},
    // Row 8: 1/10, 1/11, 1/12, 1/13, 1/14, 1/15, 1/16, 1/17, 1/18, 1/19, 1/20, 1/21
    {{
        uint256{0xcccccccccccccccbULL, 0xccccccccccccccccULL, 0xccccccccccccccccULL, 0x0cccccccccccccccULL},
        uint256{0xe8ba2e8ba2e8ba26ULL, 0x2e8ba2e8ba2e8ba2ULL, 0xa2e8ba2e8ba2e8baULL, 0x3a2e8ba2e8ba2e8bULL},
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
        uint256{0xddddddddddddddd0ULL, 0xddddddddddddddddULL, 0xddddddddddddddddULL, 0x5dddddddddddddddULL},
        uint256{0xfffffffffffffff3ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x57ffffffffffffffULL},
        uint256{0x5a5a5a5a5a5a5a4dULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL},
        uint256{0xe38e38e38e38e389ULL, 0x8e38e38e38e38e38ULL, 0x38e38e38e38e38e3ULL, 0x238e38e38e38e38eULL},
        uint256{0x86bca1af286bca14ULL, 0xbca1af286bca1af2ULL, 0xa1af286bca1af286ULL, 0x2f286bca1af286bcULL},
        uint256{0x666666666666665cULL, 0x6666666666666666ULL, 0x6666666666666666ULL, 0x4666666666666666ULL},
        uint256{0x0c30c30c30c30c2fULL, 0x30c30c30c30c30c3ULL, 0xc30c30c30c30c30cULL, 0x0c30c30c30c30c30ULL},
    }},
    // Row 9: 1/11, 1/12, 1/13, 1/14, 1/15, 1/16, 1/17, 1/18, 1/19, 1/20, 1/21, 1/22
    {{
        uint256{0xe8ba2e8ba2e8ba26ULL, 0x2e8ba2e8ba2e8ba2ULL, 0xa2e8ba2e8ba2e8baULL, 0x3a2e8ba2e8ba2e8bULL},
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
        uint256{0xddddddddddddddd0ULL, 0xddddddddddddddddULL, 0xddddddddddddddddULL, 0x5dddddddddddddddULL},
        uint256{0xfffffffffffffff3ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x57ffffffffffffffULL},
        uint256{0x5a5a5a5a5a5a5a4dULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL},
        uint256{0xe38e38e38e38e389ULL, 0x8e38e38e38e38e38ULL, 0x38e38e38e38e38e3ULL, 0x238e38e38e38e38eULL},
        uint256{0x86bca1af286bca14ULL, 0xbca1af286bca1af2ULL, 0xa1af286bca1af286ULL, 0x2f286bca1af286bcULL},
        uint256{0x666666666666665cULL, 0x6666666666666666ULL, 0x6666666666666666ULL, 0x4666666666666666ULL},
        uint256{0x0c30c30c30c30c2fULL, 0x30c30c30c30c30c3ULL, 0xc30c30c30c30c30cULL, 0x0c30c30c30c30c30ULL},
        uint256{0x745d1745d1745d13ULL, 0x1745d1745d1745d1ULL, 0xd1745d1745d1745dULL, 0x1d1745d1745d1745ULL},
    }},
    // Row 10: 1/12, 1/13, 1/14, 1/15, 1/16, 1/17, 1/18, 1/19, 1/20, 1/21, 1/22, 1/23
    {{
        uint256{0x5555555555555544ULL, 0x5555555555555555ULL, 0x5555555555555555ULL, 0x7555555555555555ULL},
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
        uint256{0xddddddddddddddd0ULL, 0xddddddddddddddddULL, 0xddddddddddddddddULL, 0x5dddddddddddddddULL},
        uint256{0xfffffffffffffff3ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x57ffffffffffffffULL},
        uint256{0x5a5a5a5a5a5a5a4dULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL},
        uint256{0xe38e38e38e38e389ULL, 0x8e38e38e38e38e38ULL, 0x38e38e38e38e38e3ULL, 0x238e38e38e38e38eULL},
        uint256{0x86bca1af286bca14ULL, 0xbca1af286bca1af2ULL, 0xa1af286bca1af286ULL, 0x2f286bca1af286bcULL},
        uint256{0x666666666666665cULL, 0x6666666666666666ULL, 0x6666666666666666ULL, 0x4666666666666666ULL},
        uint256{0x0c30c30c30c30c2fULL, 0x30c30c30c30c30c3ULL, 0xc30c30c30c30c30cULL, 0x0c30c30c30c30c30ULL},
        uint256{0x745d1745d1745d13ULL, 0x1745d1745d1745d1ULL, 0xd1745d1745d1745dULL, 0x1d1745d1745d1745ULL},
        uint256{0xe9bd37a6f4de9bc3ULL, 0xa6f4de9bd37a6f4dULL, 0x9bd37a6f4de9bd37ULL, 0x6f4de9bd37a6f4deULL},
    }},
    // Row 11: 1/13, 1/14, 1/15, 1/16, 1/17, 1/18, 1/19, 1/20, 1/21, 1/22, 1/23, 1/24
    {{
        uint256{0x3b13b13b13b13b0bULL, 0x13b13b13b13b13b1ULL, 0xb13b13b13b13b13bULL, 0x3b13b13b13b13b13ULL},
        uint256{0x924924924924923dULL, 0x4924924924924924ULL, 0x2492492492492492ULL, 0x5249249249249249ULL},
        uint256{0xddddddddddddddd0ULL, 0xddddddddddddddddULL, 0xddddddddddddddddULL, 0x5dddddddddddddddULL},
        uint256{0xfffffffffffffff3ULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL, 0x57ffffffffffffffULL},
        uint256{0x5a5a5a5a5a5a5a4dULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL, 0x5a5a5a5a5a5a5a5aULL},
        uint256{0xe38e38e38e38e389ULL, 0x8e38e38e38e38e38ULL, 0x38e38e38e38e38e3ULL, 0x238e38e38e38e38eULL},
        uint256{0x86bca1af286bca14ULL, 0xbca1af286bca1af2ULL, 0xa1af286bca1af286ULL, 0x2f286bca1af286bcULL},
        uint256{0x666666666666665cULL, 0x6666666666666666ULL, 0x6666666666666666ULL, 0x4666666666666666ULL},
        uint256{0x0c30c30c30c30c2fULL, 0x30c30c30c30c30c3ULL, 0xc30c30c30c30c30cULL, 0x0c30c30c30c30c30ULL},
        uint256{0x745d1745d1745d13ULL, 0x1745d1745d1745d1ULL, 0xd1745d1745d1745dULL, 0x1d1745d1745d1745ULL},
        uint256{0xe9bd37a6f4de9bc3ULL, 0xa6f4de9bd37a6f4dULL, 0x9bd37a6f4de9bd37ULL, 0x6f4de9bd37a6f4deULL},
        uint256{0xaaaaaaaaaaaaaaa2ULL, 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL, 0x3aaaaaaaaaaaaaaaULL},
    }},
}};

// Flags to indicate that precomputed MDS matrices are available
inline constexpr bool HAS_PRECOMPUTED_MDS_5 = true;
inline constexpr bool HAS_PRECOMPUTED_MDS_12 = true;

/**
 * @brief Check if a precomputed MDS matrix is available for the given size.
 */
[[nodiscard]] inline constexpr bool has_precomputed_mds(size_t size) noexcept {
    return (size == 5 && HAS_PRECOMPUTED_MDS_5) || (size == 12 && HAS_PRECOMPUTED_MDS_12);
}

}  // namespace rescue::mds
