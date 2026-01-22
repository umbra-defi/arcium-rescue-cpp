#pragma once

/**
 * @file rescue_desc.hpp
 * @brief Rescue cipher/hash description and permutation.
 *
 * This file provides the RescueDesc class which contains all parameters
 * and core operations for the Rescue cipher and hash function.
 *
 * See: https://tosc.iacr.org/index.php/ToSC/article/view/8695/8287
 */

#include <rescue/field.hpp>
#include <rescue/matrix.hpp>

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace rescue {

// ============================================================================
// Mode definitions
// ============================================================================

/**
 * @brief Cipher mode configuration.
 * @param key The cipher key as field elements.
 */
struct CipherMode {
    std::vector<Fp> key;
};

/**
 * @brief Hash mode configuration.
 * @param m Total state size (rate + capacity).
 * @param capacity Capacity (hidden state) size.
 */
struct HashMode {
    size_t m;
    size_t capacity;
};

/**
 * @brief Rescue operation mode.
 */
using RescueMode = std::variant<CipherMode, HashMode>;

// ============================================================================
// Security levels
// ============================================================================

/// Security level for block cipher (bits)
constexpr size_t SECURITY_LEVEL_BLOCK_CIPHER = 128;

/// Security level for hash function (bits)
constexpr size_t SECURITY_LEVEL_HASH_FUNCTION = 256;

// ============================================================================
// RescueDesc class
// ============================================================================

/**
 * @brief Description and parameters for the Rescue cipher or hash function.
 *
 * This class computes and stores all parameters needed for Rescue operations:
 * - Alpha and its inverse (S-box exponents)
 * - Number of rounds
 * - MDS matrix and its inverse
 * - Round constants/keys
 */
class RescueDesc {
public:
    /**
     * @brief Construct a RescueDesc for cipher mode.
     * @param key The cipher key as field elements.
     */
    explicit RescueDesc(const std::vector<Fp>& key);

    /**
     * @brief Construct a RescueDesc for hash mode.
     * @param m Total state size (rate + capacity).
     * @param capacity Capacity size.
     */
    RescueDesc(size_t m, size_t capacity);

    // Default copy/move operations
    RescueDesc(const RescueDesc&) = default;
    RescueDesc(RescueDesc&&) noexcept = default;
    RescueDesc& operator=(const RescueDesc&) = default;
    RescueDesc& operator=(RescueDesc&&) noexcept = default;
    ~RescueDesc() = default;

    // Accessors

    /**
     * @brief Get the operation mode.
     */
    [[nodiscard]] const RescueMode& mode() const { return mode_; }

    /**
     * @brief Check if this is cipher mode.
     */
    [[nodiscard]] bool is_cipher() const { return std::holds_alternative<CipherMode>(mode_); }

    /**
     * @brief Check if this is hash mode.
     */
    [[nodiscard]] bool is_hash() const { return std::holds_alternative<HashMode>(mode_); }

    /**
     * @brief Get the state size m.
     */
    [[nodiscard]] size_t m() const { return m_; }

    /**
     * @brief Get alpha (S-box exponent).
     */
    [[nodiscard]] const mpz_class& alpha() const { return alpha_; }

    /**
     * @brief Get alpha inverse.
     */
    [[nodiscard]] const mpz_class& alpha_inverse() const { return alpha_inverse_; }

    /**
     * @brief Get number of rounds.
     */
    [[nodiscard]] size_t n_rounds() const { return n_rounds_; }

    /**
     * @brief Get the MDS matrix.
     */
    [[nodiscard]] const Matrix& mds_matrix() const { return mds_mat_; }

    /**
     * @brief Get the inverse MDS matrix.
     */
    [[nodiscard]] const Matrix& mds_matrix_inverse() const { return mds_mat_inverse_; }

    /**
     * @brief Get the round keys.
     */
    [[nodiscard]] const std::vector<Matrix>& round_keys() const { return round_keys_; }

    // Permutation operations

    /**
     * @brief Apply the Rescue permutation to a state.
     * @param state The input state as a column vector matrix.
     * @return The permuted state.
     */
    [[nodiscard]] Matrix permute(const Matrix& state) const;

    /**
     * @brief Apply the inverse Rescue permutation to a state.
     * @param state The input state as a column vector matrix.
     * @return The inverse-permuted state.
     */
    [[nodiscard]] Matrix permute_inverse(const Matrix& state) const;

private:
    RescueMode mode_;
    size_t m_;

    mpz_class alpha_;
    mpz_class alpha_inverse_;
    size_t n_rounds_;

    Matrix mds_mat_;
    Matrix mds_mat_inverse_;
    std::vector<Matrix> round_keys_;

    /**
     * @brief Initialize common parameters.
     */
    void init_common();

    /**
     * @brief Sample round constants using SHAKE256.
     * @return Vector of round constant matrices.
     */
    [[nodiscard]] std::vector<Matrix> sample_constants();

    /**
     * @brief Compute the key schedule for cipher mode.
     * @param constants The round constants from sample_constants().
     * @param key The cipher key as a column vector.
     * @return The round keys.
     */
    [[nodiscard]] std::vector<Matrix> compute_key_schedule(const std::vector<Matrix>& constants,
                                                           const Matrix& key) const;
};

// ============================================================================
// Helper functions
// ============================================================================

/**
 * @brief Find alpha and its inverse for a given field modulus.
 *
 * Alpha is the smallest prime that does not divide (p-1).
 *
 * @param p The field modulus.
 * @return Pair of (alpha, alpha_inverse).
 */
[[nodiscard]] std::pair<mpz_class, mpz_class> get_alpha_and_inverse(const mpz_class& p);

/**
 * @brief Calculate the number of rounds needed for security.
 * @param mode The operation mode.
 * @param alpha The S-box exponent.
 * @param m The state size.
 * @return Number of rounds.
 */
[[nodiscard]] size_t get_n_rounds(const RescueMode& mode, const mpz_class& alpha, size_t m);

/**
 * @brief Build a Cauchy MDS matrix.
 * @param size The matrix dimension.
 * @return The Cauchy matrix.
 */
[[nodiscard]] Matrix build_cauchy_matrix(size_t size);

/**
 * @brief Build the inverse of a Cauchy MDS matrix.
 * @param size The matrix dimension.
 * @return The inverse Cauchy matrix.
 */
[[nodiscard]] Matrix build_inverse_cauchy_matrix(size_t size);

/**
 * @brief Apply the Rescue permutation and return all intermediate states.
 * @param mode The operation mode.
 * @param alpha The S-box exponent.
 * @param alpha_inverse The inverse S-box exponent.
 * @param mds_mat The MDS matrix.
 * @param subkeys The round keys.
 * @param state The initial state.
 * @return Vector of all intermediate states.
 */
[[nodiscard]] std::vector<Matrix> rescue_permutation(const RescueMode& mode,
                                                     const mpz_class& alpha,
                                                     const mpz_class& alpha_inverse,
                                                     const Matrix& mds_mat,
                                                     const std::vector<Matrix>& subkeys,
                                                     const Matrix& state);

/**
 * @brief Apply the inverse Rescue permutation and return all intermediate states.
 * @param mode The operation mode.
 * @param alpha The S-box exponent.
 * @param alpha_inverse The inverse S-box exponent.
 * @param mds_mat_inverse The inverse MDS matrix.
 * @param subkeys The round keys.
 * @param state The initial state.
 * @return Vector of all intermediate states.
 */
[[nodiscard]] std::vector<Matrix> rescue_permutation_inverse(const RescueMode& mode,
                                                              const mpz_class& alpha,
                                                              const mpz_class& alpha_inverse,
                                                              const Matrix& mds_mat_inverse,
                                                              const std::vector<Matrix>& subkeys,
                                                              const Matrix& state);

}  // namespace rescue
