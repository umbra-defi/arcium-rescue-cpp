#include <rescue/rescue_desc.hpp>

#include <rescue/constant_time.hpp>
#include <rescue/utils.hpp>

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace rescue {

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

    // Build MDS matrices
    mds_mat_ = build_cauchy_matrix(m_);
    mds_mat_inverse_ = build_inverse_cauchy_matrix(m_);

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

std::vector<Matrix> RescueDesc::sample_constants() {
    // Buffer length for field elements (add 16 bytes for uniform distribution)
    size_t buffer_len = (Fp::BITS + 7) / 8 + 16;

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
            mpz_class value = deserialize_le(chunk);
            r_field_array.emplace_back(value);
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
                    row.emplace_back(deserialize_le(chunk));
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
        seed_str << "Rescue-XLIX(" << Fp::P << "," << m_ << "," 
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
                data.emplace_back(deserialize_le(chunk));
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

std::pair<mpz_class, mpz_class> get_alpha_and_inverse(const mpz_class& p) {
    mpz_class p_minus_one = p - 1;

    // Primes to try
    static const mpz_class primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};

    mpz_class alpha = 0;
    for (const auto& a : primes) {
        if (p_minus_one % a != 0) {
            alpha = a;
            break;
        }
    }

    if (alpha == 0) {
        throw std::runtime_error("Could not find prime alpha that does not divide p-1");
    }

    // Compute alpha_inverse = alpha^(-1) mod (p-1)
    mpz_class alpha_inverse;
    if (mpz_invert(alpha_inverse.get_mpz_t(), alpha.get_mpz_t(), p_minus_one.get_mpz_t()) == 0) {
        throw std::runtime_error("Failed to compute alpha inverse");
    }

    return {alpha, alpha_inverse};
}

size_t get_n_rounds(const RescueMode& mode, const mpz_class& alpha, size_t m) {
    double log2_p = static_cast<double>(mpz_sizeinbase(Fp::P.get_mpz_t(), 2));
    double alpha_d = alpha.get_d();

    if (std::holds_alternative<CipherMode>(mode)) {
        // Block cipher rounds calculation
        double l0_d = (2.0 * SECURITY_LEVEL_BLOCK_CIPHER) /
                      ((static_cast<double>(m) + 1.0) * (log2_p - std::log2(alpha_d - 1.0)));
        size_t l0 = static_cast<size_t>(std::ceil(l0_d));

        size_t l1;
        if (alpha == 3) {
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

        // Binomial coefficient calculation
        auto binomial = [](size_t n, size_t k) -> mpz_class {
            mpz_class result;
            mpz_bin_uiui(result.get_mpz_t(), static_cast<unsigned long>(n),
                         static_cast<unsigned long>(k));
            return result;
        };

        mpz_class target = mpz_class(1) << SECURITY_LEVEL_HASH_FUNCTION;

        size_t l1 = 1;
        mpz_class tmp = binomial(v_func(l1) + dcon(l1), v_func(l1));

        while (tmp * tmp <= target && l1 <= 23) {
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
            Fp sum(static_cast<unsigned long>(i + j));
            row.push_back(sum.inv());
        }
        data.push_back(std::move(row));
    }

    return Matrix(data);
}

Matrix build_inverse_cauchy_matrix(size_t size) {
    // Helper to compute product of field elements
    auto product = [](const std::vector<mpz_class>& arr) -> Fp {
        Fp result = Fp::ONE;
        for (const auto& val : arr) {
            result = result * Fp(val);
        }
        return result;
    };

    // Helper to compute product with one element excluded (as difference)
    auto prime_product = [](const std::vector<mpz_class>& arr, const mpz_class& exclude) -> Fp {
        Fp result = Fp::ONE;
        for (const auto& u : arr) {
            if (u != exclude) {
                result = result * Fp(exclude - u);
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
            mpz_class i_val(static_cast<long>(i));
            mpz_class j_val(static_cast<long>(j));

            // Build arrays for products
            std::vector<mpz_class> neg_range;  // [-i - 1, -i - 2, ..., -i - size]
            for (size_t k = 1; k <= size; ++k) {
                neg_range.push_back(-i_val - mpz_class(static_cast<long>(k)));
            }

            std::vector<mpz_class> pos_range;  // [1, 2, ..., size]
            for (size_t k = 1; k <= size; ++k) {
                pos_range.push_back(mpz_class(static_cast<long>(k)));
            }

            std::vector<mpz_class> j_plus_range;  // [j + 1, j + 2, ..., j + size]
            for (size_t k = 1; k <= size; ++k) {
                j_plus_range.push_back(j_val + mpz_class(static_cast<long>(k)));
            }

            std::vector<mpz_class> neg_only;  // [-1, -2, ..., -size]
            for (size_t k = 1; k <= size; ++k) {
                neg_only.push_back(mpz_class(-static_cast<long>(k)));
            }

            Fp a = product(neg_range);
            Fp a_prime = prime_product(pos_range, j_val);
            Fp b = product(j_plus_range);
            Fp b_prime = prime_product(neg_only, -i_val);

            Fp denominator = a_prime * b_prime * Fp(-i_val - j_val);
            Fp result = a * b * denominator.inv();
            row.push_back(result);
        }
        data.push_back(std::move(row));
    }

    return Matrix(data);
}

namespace {

// Get exponent for even rounds
mpz_class exponent_for_even(const RescueMode& mode, const mpz_class& alpha,
                            const mpz_class& alpha_inverse) {
    if (std::holds_alternative<CipherMode>(mode)) {
        return alpha_inverse;
    }
    return alpha;
}

// Get exponent for odd rounds
mpz_class exponent_for_odd(const RescueMode& mode, const mpz_class& alpha,
                           const mpz_class& alpha_inverse) {
    if (std::holds_alternative<CipherMode>(mode)) {
        return alpha;
    }
    return alpha_inverse;
}

}  // namespace

std::vector<Matrix> rescue_permutation(const RescueMode& mode,
                                       const mpz_class& alpha,
                                       const mpz_class& alpha_inverse,
                                       const Matrix& mds_mat,
                                       const std::vector<Matrix>& subkeys,
                                       const Matrix& state) {
    mpz_class exp_even = exponent_for_even(mode, alpha, alpha_inverse);
    mpz_class exp_odd = exponent_for_odd(mode, alpha, alpha_inverse);

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
                                                const mpz_class& alpha,
                                                const mpz_class& alpha_inverse,
                                                const Matrix& mds_mat_inverse,
                                                const std::vector<Matrix>& subkeys,
                                                const Matrix& state) {
    mpz_class exp_even = exponent_for_even(mode, alpha, alpha_inverse);
    mpz_class exp_odd = exponent_for_odd(mode, alpha, alpha_inverse);

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
