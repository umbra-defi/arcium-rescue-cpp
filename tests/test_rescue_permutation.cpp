/**
 * @file test_rescue_permutation.cpp
 * @brief Unit tests for Rescue permutation and RescueDesc.
 */

#include <rescue/rescue_desc.hpp>

#include <gtest/gtest.h>

using namespace rescue;

class RescueDescTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple cipher key
        cipher_key = {Fp(uint64_t{1}), Fp(uint64_t{2}), Fp(uint64_t{3}),
                      Fp(uint64_t{4}), Fp(uint64_t{5})};
    }

    std::vector<Fp> cipher_key;
};

TEST_F(RescueDescTest, CipherModeConstruction) {
    RescueDesc desc(cipher_key);

    EXPECT_TRUE(desc.is_cipher());
    EXPECT_FALSE(desc.is_hash());
    EXPECT_EQ(desc.m(), cipher_key.size());

    // Alpha should be 5 for Curve25519 field (smallest prime not dividing p-1)
    EXPECT_EQ(desc.alpha(), 5);

    // Check that matrices are initialized
    EXPECT_EQ(desc.mds_matrix().rows(), cipher_key.size());
    EXPECT_EQ(desc.mds_matrix().cols(), cipher_key.size());

    // Round keys should be populated
    EXPECT_FALSE(desc.round_keys().empty());
}

TEST_F(RescueDescTest, HashModeConstruction) {
    RescueDesc desc(12, 5);  // m=12, capacity=5 (rate=7)

    EXPECT_FALSE(desc.is_cipher());
    EXPECT_TRUE(desc.is_hash());
    EXPECT_EQ(desc.m(), 12);
}

TEST_F(RescueDescTest, AlphaAndInverse) {
    auto [alpha, alpha_inv] = get_alpha_and_inverse(Fp::P);

    // Alpha should be 5 for Curve25519
    EXPECT_EQ(alpha, 5);

    // Verify: alpha * alpha_inverse ≡ 1 (mod p-1)
    mpz_class product = (alpha * alpha_inv) % (Fp::P - 1);
    EXPECT_EQ(product, 1);
}

TEST_F(RescueDescTest, CauchyMatrix) {
    size_t size = 5;
    Matrix cauchy = build_cauchy_matrix(size);

    EXPECT_EQ(cauchy.rows(), size);
    EXPECT_EQ(cauchy.cols(), size);

    // Cauchy matrix should be invertible (non-zero determinant)
    EXPECT_FALSE(cauchy.det().is_zero());
}

TEST_F(RescueDescTest, CauchyInverse) {
    size_t size = 5;
    Matrix cauchy = build_cauchy_matrix(size);
    Matrix cauchy_inv = build_inverse_cauchy_matrix(size);

    // M * M^(-1) should equal identity
    Matrix product = cauchy.mat_mul(cauchy_inv);
    Matrix identity = Matrix::identity(size);

    for (size_t i = 0; i < size; ++i) {
        for (size_t j = 0; j < size; ++j) {
            EXPECT_EQ(product(i, j), identity(i, j))
                << "Mismatch at (" << i << ", " << j << ")";
        }
    }
}

TEST_F(RescueDescTest, PermutationRoundtrip) {
    RescueDesc desc(cipher_key);

    // Create test state
    std::vector<Fp> state_data;
    for (size_t i = 0; i < cipher_key.size(); ++i) {
        state_data.push_back(Fp(static_cast<uint64_t>(i + 10)));
    }
    Matrix state(state_data);

    // Apply permutation
    Matrix permuted = desc.permute(state);

    // Verify permutation changed the state
    EXPECT_NE(permuted, state);

    // Apply inverse permutation
    Matrix recovered = desc.permute_inverse(permuted);

    // Should recover original state
    for (size_t i = 0; i < cipher_key.size(); ++i) {
        EXPECT_EQ(recovered(i, 0), state(i, 0))
            << "Mismatch at index " << i;
    }
}

TEST_F(RescueDescTest, HashPermutationDeterministic) {
    RescueDesc desc1(12, 5);
    RescueDesc desc2(12, 5);

    // Create test state
    std::vector<Fp> state_data(12, Fp::ZERO);
    state_data[0] = Fp(uint64_t{42});
    Matrix state(state_data);

    // Both should produce same result
    Matrix result1 = desc1.permute(state);
    Matrix result2 = desc2.permute(state);

    EXPECT_EQ(result1, result2);
}

TEST_F(RescueDescTest, RoundConstantsGeneration) {
    RescueDesc cipher_desc(cipher_key);
    RescueDesc hash_desc(12, 5);

    // Cipher mode should have round keys derived from key schedule
    EXPECT_GT(cipher_desc.round_keys().size(), 0);

    // Hash mode should have round constants from SHAKE256
    EXPECT_GT(hash_desc.round_keys().size(), 0);

    // First hash round constant should be zero (prepended for Algorithm 3)
    const auto& first_hash_const = hash_desc.round_keys()[0];
    for (size_t i = 0; i < 12; ++i) {
        EXPECT_TRUE(first_hash_const(i, 0).is_zero());
    }
}

TEST_F(RescueDescTest, InvalidConstruction) {
    // Key too small
    EXPECT_THROW(RescueDesc(std::vector<Fp>{Fp::ONE}), std::invalid_argument);

    // Capacity >= m
    EXPECT_THROW(RescueDesc(5, 5), std::invalid_argument);
    EXPECT_THROW(RescueDesc(5, 6), std::invalid_argument);
}

TEST_F(RescueDescTest, NumberOfRounds) {
    // Cipher mode: ensure reasonable number of rounds for security
    RescueDesc cipher_desc(cipher_key);
    EXPECT_GE(cipher_desc.n_rounds(), 5);

    // Hash mode: should have at least minimum rounds
    RescueDesc hash_desc(12, 5);
    EXPECT_GE(hash_desc.n_rounds(), 5);
}

TEST_F(RescueDescTest, MDSMatrixProperties) {
    RescueDesc desc(cipher_key);
    const Matrix& mds = desc.mds_matrix();

    // MDS matrix should be square
    EXPECT_TRUE(mds.is_square());
    EXPECT_EQ(mds.rows(), cipher_key.size());

    // MDS matrix should be invertible
    EXPECT_FALSE(mds.det().is_zero());
}

TEST_F(RescueDescTest, PermutationWithRandomInput) {
    RescueDesc desc(cipher_key);

    // Run multiple times with random inputs
    for (int trial = 0; trial < 10; ++trial) {
        std::vector<Fp> state_data;
        for (size_t i = 0; i < cipher_key.size(); ++i) {
            state_data.push_back(Fp::random());
        }
        Matrix state(state_data);

        // Permutation and inverse should roundtrip
        Matrix permuted = desc.permute(state);
        Matrix recovered = desc.permute_inverse(permuted);

        for (size_t i = 0; i < cipher_key.size(); ++i) {
            EXPECT_EQ(recovered(i, 0), state(i, 0))
                << "Trial " << trial << ", index " << i;
        }
    }
}
