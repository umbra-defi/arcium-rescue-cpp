#include <rescue/rescue_desc.hpp>

#include <rescue/mds_precomputed.hpp>
#include <rescue/utils.hpp>

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace rescue {

// ============================================================================
// Helper: Binomial coefficient calculation
// ============================================================================

namespace {

/**
 * @brief Compute binomial coefficient C(n, k) as uint256.
 *
 * Uses the multiplicative formula: C(n,k) = prod_{i=1}^{k} (n-k+i)/i
 * with careful ordering to avoid intermediate overflow.
 */
[[nodiscard]] uint256 binomial(size_t n, size_t k) {
    if (k > n) {
        return uint256::zero();
    }
    if (k == 0 || k == n) {
        return uint256::one();
    }

    // Use the smaller of k and n-k for efficiency
    if (k > n - k) {
        k = n - k;
    }

    // Compute using uint256 arithmetic
    // C(n,k) = (n * (n-1) * ... * (n-k+1)) / (k * (k-1) * ... * 1)
    uint256 result = uint256::one();

    for (size_t i = 0; i < k; ++i) {
        // Multiply by (n - i)
        uint256 numerator(static_cast<uint64_t>(n - i));
        uint512 wide = mul_wide(result, numerator);
        result = wide.low();  // No reduction needed, intermediate values

        // Divide by (i + 1)
        // Since we're computing a binomial coefficient, the division is always exact
        uint64_t divisor = i + 1;

        // Long division by a single limb
        uint64_t remainder = 0;
        for (int j = uint256::LIMBS - 1; j >= 0; --j) {
#if defined(__SIZEOF_INT128__)
            unsigned __int128 dividend = (static_cast<unsigned __int128>(remainder) << 64) | result.limb(j);
            result.limb(j) = static_cast<uint64_t>(dividend / divisor);
            remainder = static_cast<uint64_t>(dividend % divisor);
#else
            // Portable version
            uint64_t high = (remainder << 32) | (result.limb(j) >> 32);
            uint64_t q_high = high / divisor;
            uint64_t r_high = high % divisor;

            uint64_t low = (r_high << 32) | (result.limb(j) & 0xFFFFFFFF);
            uint64_t q_low = low / divisor;
            remainder = low % divisor;

            result.limb(j) = (q_high << 32) | q_low;
#endif
        }
    }

    return result;
}

/**
 * @brief Check if a divides b (b % a == 0) for uint256.
 */
[[nodiscard]] bool divides(uint64_t a, const uint256& b) {
    // Division by single limb
    uint64_t remainder = 0;
    for (int i = uint256::LIMBS - 1; i >= 0; --i) {
#if defined(__SIZEOF_INT128__)
        unsigned __int128 dividend = (static_cast<unsigned __int128>(remainder) << 64) | b.limb(i);
        remainder = static_cast<uint64_t>(dividend % a);
#else
        uint64_t high = (remainder << 32) | (b.limb(i) >> 32);
        uint64_t r_high = high % a;
        uint64_t low = (r_high << 32) | (b.limb(i) & 0xFFFFFFFF);
        remainder = low % a;
#endif
    }
    return remainder == 0;
}

/**
 * @brief Extended Euclidean algorithm to find modular inverse.
 * Returns x such that a*x ≡ 1 (mod m)
 */
[[nodiscard]] uint256 mod_inverse_extended(uint64_t a, const uint256& m) {
    // For small 'a', we can use a simpler approach
    // We need to find x such that a * x ≡ 1 (mod m)
    // Since a is small, we can compute a^(m-2) mod m using Fermat's little theorem
    // But m here is p-1, not necessarily prime

    // Use extended Euclidean algorithm
    // For simplicity with uint256, we'll use the binary extended GCD

    // Since a is small (< 50), and m is large, we can simplify
    // The modular inverse exists iff gcd(a, m) = 1

    // Use iterative extended Euclidean algorithm
    // Adapted for uint256

    uint256 old_r = m;
    uint256 r = uint256(a);
    uint256 old_s = uint256::zero();
    uint256 s = uint256::one();

    while (!r.is_zero()) {
        // Compute quotient q = old_r / r
        // This is expensive for uint256, but necessary

        // Simple division for this specific case
        // We know a is small, so r will shrink quickly

        // Compute old_r / r using long division
        uint256 q = uint256::zero();
        uint256 temp_dividend = old_r;

        // Find the highest bit position of divisor (r)
        size_t r_bits = r.bit_length();
        size_t dividend_bits = temp_dividend.bit_length();

        if (dividend_bits >= r_bits) {
            size_t shift = dividend_bits - r_bits;

            while (shift < 256) {
                uint256 shifted_r = r << shift;
                if (shifted_r <= temp_dividend) {
                    temp_dividend = temp_dividend - shifted_r;
                    q.set_bit(shift);
                }
                if (shift == 0) break;
                --shift;
            }
        }

        // Now q = old_r / r, temp_dividend = old_r % r

        // Update: (old_r, r) = (r, old_r - q * r) = (r, temp_dividend)
        old_r = r;
        r = temp_dividend;

        // Update: (old_s, s) = (s, old_s - q * s)
        // s_new = old_s - q * s
        // Need to handle potential underflow (work in signed arithmetic)
        uint512 qs_wide = mul_wide(q, s);
        uint256 qs = qs_wide.low();  // Truncate to 256 bits

        // Check if old_s >= qs
        if (old_s >= qs) {
            uint256 new_s = old_s - qs;
            old_s = s;
            s = new_s;
        } else {
            // old_s - qs is negative, so we need to add m
            // new_s = old_s - qs + m (mod m)
            uint256 diff = qs - old_s;
            // new_s = m - diff (mod m)
            uint256 new_s;
            if (diff < m) {
                new_s = m - diff;
            } else {
                // diff >= m, need modular reduction
                // This shouldn't happen for our use case
                new_s = uint256::zero();
            }
            old_s = s;
            s = new_s;
        }
    }

    // At this point, old_r should be 1 (gcd = 1)
    // and old_s is the inverse

    // Ensure result is in [0, m)
    if (old_s >= m) {
        return old_s - m;
    }
    return old_s;
}

}  // anonymous namespace

// ============================================================================
// RescueDesc implementation
// ============================================================================

RescueDesc::RescueDesc(const std::vector<Fp>& key) : mode_(CipherMode{key}), m_(key.size()) {
    if (m_ < 2) {
        throw std::invalid_argument("Cipher key must have at least 2 elements");
    }
    init_common();
}

RescueDesc::RescueDesc(size_t m, size_t capacity) : mode_(HashMode{m, capacity}), m_(m) {
    if (m <= capacity) {
        throw std::invalid_argument("State size m must be greater than capacity");
    }
    init_common();
}

void RescueDesc::init_common() {
    // Get alpha and alpha_inverse
    auto [a, a_inv] = get_alpha_and_inverse(Fp::P);
    alpha_ = a;
    alpha_inverse_ = a_inv;

    // Get number of rounds
    n_rounds_ = get_n_rounds(mode_, alpha_, m_);

    // Build MDS matrices - use precomputed if available
    if (m_ == 5 && mds::HAS_PRECOMPUTED_MDS_5) {
        // Use precomputed 5x5 MDS matrix for cipher mode
        std::vector<std::vector<Fp>> mds_data;
        mds_data.reserve(5);
        for (size_t i = 0; i < 5; ++i) {
            std::vector<Fp> row;
            row.reserve(5);
            for (size_t j = 0; j < 5; ++j) {
                row.emplace_back(mds::MDS_5x5[i][j]);
            }
            mds_data.push_back(std::move(row));
        }
        mds_mat_ = Matrix(mds_data);
        mds_mat_inverse_ = build_inverse_cauchy_matrix(m_);
    } else if (m_ == 12 && mds::HAS_PRECOMPUTED_MDS_12) {
        // Use precomputed 12x12 MDS matrix for hash mode
        std::vector<std::vector<Fp>> mds_data;
        mds_data.reserve(12);
        for (size_t i = 0; i < 12; ++i) {
            std::vector<Fp> row;
            row.reserve(12);
            for (size_t j = 0; j < 12; ++j) {
                row.emplace_back(mds::MDS_12x12[i][j]);
            }
            mds_data.push_back(std::move(row));
        }
        mds_mat_ = Matrix(mds_data);
        mds_mat_inverse_ = build_inverse_cauchy_matrix(m_);
    } else {
        // Compute dynamically for non-standard sizes
        mds_mat_ = build_cauchy_matrix(m_);
        mds_mat_inverse_ = build_inverse_cauchy_matrix(m_);
    }

    // Sample round constants
    auto round_constants = sample_constants();

    // Compute round keys based on mode
    if (is_cipher()) {
        const auto& cipher_mode = std::get<CipherMode>(mode_);
        Matrix key_vec(cipher_mode.key);
        round_keys_ = compute_key_schedule(round_constants, key_vec);
    } else {
        round_keys_ = std::move(round_constants);
    }
}

/**
 * @brief Convert a wide byte array (e.g., 48 bytes) to a field element.
 * 
 * This reads all bytes as a little-endian integer and reduces mod p.
 * This is necessary for proper interoperability with the JS library
 * which uses arbitrary-precision integers.
 */
static Fp wide_bytes_to_fp(std::span<const uint8_t> bytes) {
    // For 48 bytes = 384 bits, we need to reduce a value that could be up to 2^384
    // We use the property: 2^255 ≡ 19 (mod p)
    // 
    // Split the input into: low 255 bits + high 129 bits
    // result = low + high * 2^255 ≡ low + high * 19 (mod p)
    
    if (bytes.size() <= 32) {
        // Short path: fits in uint256
        return Fp(deserialize_le(bytes));
    }
    
    // Read all bytes into limbs (up to 6 limbs for 48 bytes)
    std::array<uint64_t, 8> wide_limbs = {0};
    size_t n_limbs = (bytes.size() + 7) / 8;
    if (n_limbs > 8) n_limbs = 8;
    
    for (size_t i = 0; i < bytes.size() && i < 64; ++i) {
        wide_limbs[i / 8] |= static_cast<uint64_t>(bytes[i]) << ((i % 8) * 8);
    }
    
    // Extract low 256 bits (4 limbs)
    uint256 low{wide_limbs[0], wide_limbs[1], wide_limbs[2], wide_limbs[3]};
    
    // Extract high bits (remaining limbs)
    uint256 high{wide_limbs[4], wide_limbs[5], 0, 0};
    
    // Reduce: low + high * 2^256
    // Since 2^256 = 2 * 2^255 = 2 * 19 = 38 (mod p)
    // result = low + high * 38 (mod p)
    Fp result = Fp(low) + Fp(high) * Fp(uint64_t{38});
    
    return result;
}

std::vector<Matrix> RescueDesc::sample_constants() {
    // Buffer length for field elements (add 16 bytes for uniform distribution)
    size_t buffer_len = (Fp::BITS + 7) / 8 + 16;  // = 32 + 16 = 48 bytes

    if (is_cipher()) {
        // Cipher mode: sample matrix + vectors
        Shake256 hasher;
        hasher.update("encrypt everything, compute anything");

        size_t n_elements = m_ * m_ + 2 * m_;
        std::vector<Fp> r_field_array;
        r_field_array.reserve(n_elements);

        // We need to squeeze multiple times, but our simple implementation
        // only supports one finalization. So we squeeze all at once.
        auto randomness = hasher.finalize(n_elements * buffer_len);

        for (size_t i = 0; i < n_elements; ++i) {
            std::span<const uint8_t> chunk(randomness.data() + i * buffer_len, buffer_len);
            // Use wide_bytes_to_fp to properly handle 48-byte values
            r_field_array.push_back(wide_bytes_to_fp(chunk));
        }

        // Create matrix and vectors
        std::vector<std::vector<Fp>> mat_data;
        mat_data.reserve(m_);
        size_t idx = 0;
        for (size_t i = 0; i < m_; ++i) {
            std::vector<Fp> row;
            row.reserve(m_);
            for (size_t j = 0; j < m_; ++j) {
                row.push_back(r_field_array[idx++]);
            }
            mat_data.push_back(std::move(row));
        }
        Matrix round_constant_mat(mat_data);

        std::vector<Fp> init_data;
        init_data.reserve(m_);
        for (size_t i = 0; i < m_; ++i) {
            init_data.push_back(r_field_array[idx++]);
        }
        Matrix initial_round_constant(init_data);

        std::vector<Fp> affine_data;
        affine_data.reserve(m_);
        for (size_t i = 0; i < m_; ++i) {
            affine_data.push_back(r_field_array[idx++]);
        }
        Matrix round_constant_affine_term(affine_data);

        // Check for invertibility and resample if needed
        while (round_constant_mat.det().is_zero()) {
            // Note: This is a simplified approach. In production,
            // we'd need to continue the XOF state.
            auto new_random = random_bytes(m_ * m_ * buffer_len);
            mat_data.clear();
            for (size_t i = 0; i < m_; ++i) {
                std::vector<Fp> row;
                row.reserve(m_);
                for (size_t j = 0; j < m_; ++j) {
                    size_t offset = (i * m_ + j) * buffer_len;
                    std::span<const uint8_t> chunk(new_random.data() + offset, buffer_len);
                    // Use wide_bytes_to_fp for consistency
                    row.push_back(wide_bytes_to_fp(chunk));
                }
                mat_data.push_back(std::move(row));
            }
            round_constant_mat = Matrix(mat_data);
        }

        // Generate round constants
        std::vector<Matrix> round_constants;
        round_constants.reserve(2 * n_rounds_ + 1);
        round_constants.push_back(initial_round_constant);

        for (size_t r = 0; r < 2 * n_rounds_; ++r) {
            Matrix next = round_constant_mat.mat_mul(round_constants[r]).add(round_constant_affine_term);
            round_constants.push_back(std::move(next));
        }

        return round_constants;

    } else {
        // Hash mode
        const auto& hash_mode = std::get<HashMode>(mode_);

        std::ostringstream seed_str;
        seed_str << "Rescue-XLIX(" << Fp::P.to_string() << "," << m_ << ","
                 << hash_mode.capacity << "," << SECURITY_LEVEL_HASH_FUNCTION << ")";

        Shake256 hasher;
        hasher.update(seed_str.str());

        // Prepend a zero matrix (for Algorithm 3 from the paper)
        std::vector<Matrix> round_constants;
        round_constants.reserve(2 * n_rounds_ + 1);

        std::vector<Fp> zeros(m_, Fp::ZERO);
        round_constants.emplace_back(zeros);

        size_t n_elements = 2 * m_ * n_rounds_;
        auto randomness = hasher.finalize(n_elements * buffer_len);

        for (size_t r = 0; r < 2 * n_rounds_; ++r) {
            std::vector<Fp> data;
            data.reserve(m_);
            for (size_t i = 0; i < m_; ++i) {
                size_t offset = (r * m_ + i) * buffer_len;
                std::span<const uint8_t> chunk(randomness.data() + offset, buffer_len);
                // Use wide_bytes_to_fp to properly handle 48-byte values
                data.push_back(wide_bytes_to_fp(chunk));
            }
            round_constants.emplace_back(std::move(data));
        }

        return round_constants;
    }
}

std::vector<Matrix> RescueDesc::compute_key_schedule(const std::vector<Matrix>& constants,
                                                      const Matrix& key) const {
    return rescue_permutation(mode_, alpha_, alpha_inverse_, mds_mat_, constants, key);
}

Matrix RescueDesc::permute(const Matrix& state) const {
    auto states = rescue_permutation(mode_, alpha_, alpha_inverse_, mds_mat_, round_keys_, state);
    return states[2 * n_rounds_];
}

Matrix RescueDesc::permute_inverse(const Matrix& state) const {
    auto states = rescue_permutation_inverse(mode_, alpha_, alpha_inverse_, mds_mat_inverse_,
                                             round_keys_, state);
    return states[2 * n_rounds_];
}

// ============================================================================
// Helper functions
// ============================================================================

std::pair<uint256, uint256> get_alpha_and_inverse(const uint256& p) {
    uint256 p_minus_one = p - uint256::one();

    // Primes to try
    static const uint64_t primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};

    uint64_t alpha = 0;
    for (const auto& a : primes) {
        if (!divides(a, p_minus_one)) {
            alpha = a;
            break;
        }
    }

    if (alpha == 0) {
        throw std::runtime_error("Could not find prime alpha that does not divide p-1");
    }

    // Compute alpha_inverse = alpha^(-1) mod (p-1)
    uint256 alpha_inverse = mod_inverse_extended(alpha, p_minus_one);

    return {uint256(alpha), alpha_inverse};
}

size_t get_n_rounds(const RescueMode& mode, const uint256& alpha, size_t m) {
    double log2_p = static_cast<double>(Fp::P.bit_length());
    double alpha_d = static_cast<double>(alpha.limb(0));  // Alpha is small, fits in one limb

    if (std::holds_alternative<CipherMode>(mode)) {
        // Block cipher rounds calculation
        double l0_d = (2.0 * SECURITY_LEVEL_BLOCK_CIPHER) /
                      ((static_cast<double>(m) + 1.0) * (log2_p - std::log2(alpha_d - 1.0)));
        size_t l0 = static_cast<size_t>(std::ceil(l0_d));

        size_t l1;
        if (alpha.limb(0) == 3) {
            l1 = static_cast<size_t>(
                std::ceil((SECURITY_LEVEL_BLOCK_CIPHER + 2.0) / (4.0 * static_cast<double>(m))));
        } else {
            l1 = static_cast<size_t>(
                std::ceil((SECURITY_LEVEL_BLOCK_CIPHER + 3.0) / (5.5 * static_cast<double>(m))));
        }

        return 2 * std::max({l0, l1, size_t{5}});

    } else {
        // Hash function rounds calculation
        const auto& hash_mode = std::get<HashMode>(mode);
        size_t rate = m - hash_mode.capacity;

        auto dcon = [&](size_t n) -> size_t {
            return static_cast<size_t>(
                std::floor(0.5 * (alpha_d - 1.0) * static_cast<double>(m) *
                           static_cast<double>(n - 1) + 2.0));
        };

        auto v_func = [&](size_t n) -> size_t { return m * (n - 1) + rate; };

        uint256 target = uint256::one() << SECURITY_LEVEL_HASH_FUNCTION;

        size_t l1 = 1;
        uint256 tmp = binomial(v_func(l1) + dcon(l1), v_func(l1));

        // tmp * tmp <= target
        while (l1 <= 23) {
            uint512 tmp_sq = sqr_wide(tmp);
            // Compare tmp_sq with target
            // tmp_sq has 8 limbs, target has 4 limbs in the lower part
            // If any high limb is non-zero, tmp_sq > target
            bool tmp_sq_larger = (tmp_sq.limb(4) | tmp_sq.limb(5) | tmp_sq.limb(6) | tmp_sq.limb(7)) != 0;
            if (!tmp_sq_larger) {
                // Compare low 256 bits
                uint256 tmp_sq_low = tmp_sq.low();
                if (tmp_sq_low > target) {
                    tmp_sq_larger = true;
                }
            }

            if (tmp_sq_larger) {
                break;
            }

            l1++;
            tmp = binomial(v_func(l1) + dcon(l1), v_func(l1));
        }

        // Set minimum and add 50%
        return static_cast<size_t>(std::ceil(1.5 * static_cast<double>(std::max(size_t{5}, l1))));
    }
}

Matrix build_cauchy_matrix(size_t size) {
    std::vector<std::vector<Fp>> data;
    data.reserve(size);

    for (size_t i = 1; i <= size; ++i) {
        std::vector<Fp> row;
        row.reserve(size);
        for (size_t j = 1; j <= size; ++j) {
            // M[i][j] = 1/(i + j)
            Fp sum(static_cast<uint64_t>(i + j));
            row.push_back(sum.inv());
        }
        data.push_back(std::move(row));
    }

    return Matrix(data);
}

Matrix build_inverse_cauchy_matrix(size_t size) {
    // Helper to compute product of field elements
    auto product = [](const std::vector<int64_t>& arr) -> Fp {
        Fp result = Fp::ONE;
        for (const auto& val : arr) {
            if (val >= 0) {
                result = result * Fp(static_cast<uint64_t>(val));
            } else {
                // Negative value: compute p - |val|
                result = result * Fp(fp::P - uint256(static_cast<uint64_t>(-val)));
            }
        }
        return result;
    };

    // Helper to compute product with one element excluded (as difference)
    auto prime_product = [](const std::vector<int64_t>& arr, int64_t exclude) -> Fp {
        Fp result = Fp::ONE;
        for (const auto& u : arr) {
            if (u != exclude) {
                int64_t diff = exclude - u;
                if (diff >= 0) {
                    result = result * Fp(static_cast<uint64_t>(diff));
                } else {
                    result = result * Fp(fp::P - uint256(static_cast<uint64_t>(-diff)));
                }
            }
        }
        return result;
    };

    std::vector<std::vector<Fp>> data;
    data.reserve(size);

    for (size_t i = 1; i <= size; ++i) {
        std::vector<Fp> row;
        row.reserve(size);

        for (size_t j = 1; j <= size; ++j) {
            int64_t i_val = static_cast<int64_t>(i);
            int64_t j_val = static_cast<int64_t>(j);

            // Build arrays for products
            std::vector<int64_t> neg_range;  // [-i - 1, -i - 2, ..., -i - size]
            for (size_t k = 1; k <= size; ++k) {
                neg_range.push_back(-i_val - static_cast<int64_t>(k));
            }

            std::vector<int64_t> pos_range;  // [1, 2, ..., size]
            for (size_t k = 1; k <= size; ++k) {
                pos_range.push_back(static_cast<int64_t>(k));
            }

            std::vector<int64_t> j_plus_range;  // [j + 1, j + 2, ..., j + size]
            for (size_t k = 1; k <= size; ++k) {
                j_plus_range.push_back(j_val + static_cast<int64_t>(k));
            }

            std::vector<int64_t> neg_only;  // [-1, -2, ..., -size]
            for (size_t k = 1; k <= size; ++k) {
                neg_only.push_back(-static_cast<int64_t>(k));
            }

            Fp a = product(neg_range);
            Fp a_prime = prime_product(pos_range, j_val);
            Fp b = product(j_plus_range);
            Fp b_prime = prime_product(neg_only, -i_val);

            int64_t denom_val = -i_val - j_val;
            Fp denom_fp;
            if (denom_val >= 0) {
                denom_fp = Fp(static_cast<uint64_t>(denom_val));
            } else {
                denom_fp = Fp(fp::P - uint256(static_cast<uint64_t>(-denom_val)));
            }

            Fp denominator = a_prime * b_prime * denom_fp;
            Fp result = a * b * denominator.inv();
            row.push_back(result);
        }
        data.push_back(std::move(row));
    }

    return Matrix(data);
}

namespace {

// Get exponent for even rounds
uint256 exponent_for_even(const RescueMode& mode, const uint256& alpha,
                            const uint256& alpha_inverse) {
    if (std::holds_alternative<CipherMode>(mode)) {
        return alpha_inverse;
    }
    return alpha;
}

// Get exponent for odd rounds
uint256 exponent_for_odd(const RescueMode& mode, const uint256& alpha,
                           const uint256& alpha_inverse) {
    if (std::holds_alternative<CipherMode>(mode)) {
        return alpha;
    }
    return alpha_inverse;
}

}  // namespace

std::vector<Matrix> rescue_permutation(const RescueMode& mode,
                                       const uint256& alpha,
                                       const uint256& alpha_inverse,
                                       const Matrix& mds_mat,
                                       const std::vector<Matrix>& subkeys,
                                       const Matrix& state) {
    uint256 exp_even = exponent_for_even(mode, alpha, alpha_inverse);
    uint256 exp_odd = exponent_for_odd(mode, alpha, alpha_inverse);

    std::vector<Matrix> states;
    states.reserve(subkeys.size());

    // Initial state: state + subkeys[0]
    states.push_back(state.add(subkeys[0]));

    for (size_t r = 0; r < subkeys.size() - 1; ++r) {
        Matrix s = states[r];

        // Apply S-box (exponentiation)
        if (r % 2 == 0) {
            s = s.pow(exp_even);
        } else {
            s = s.pow(exp_odd);
        }

        // Apply MDS matrix and add round key
        states.push_back(mds_mat.mat_mul(s).add(subkeys[r + 1]));
    }

    return states;
}

std::vector<Matrix> rescue_permutation_inverse(const RescueMode& mode,
                                                const uint256& alpha,
                                                const uint256& alpha_inverse,
                                                const Matrix& mds_mat_inverse,
                                                const std::vector<Matrix>& subkeys,
                                                const Matrix& state) {
    uint256 exp_even = exponent_for_even(mode, alpha, alpha_inverse);
    uint256 exp_odd = exponent_for_odd(mode, alpha, alpha_inverse);

    std::vector<Matrix> states;
    states.reserve(subkeys.size());
    states.push_back(state);

    for (size_t r = 0; r < subkeys.size() - 1; ++r) {
        Matrix s = states[r];

        // Subtract round key and apply inverse MDS
        s = mds_mat_inverse.mat_mul(s.sub(subkeys[subkeys.size() - 1 - r]));

        // Apply inverse S-box
        if (r % 2 == 0) {
            s = s.pow(exp_even);
        } else {
            s = s.pow(exp_odd);
        }

        states.push_back(std::move(s));
    }

    // Final step: subtract first round key
    states.push_back(states.back().sub(subkeys[0]));
    states.erase(states.begin());

    return states;
}

}  // namespace rescue
