/**
 * @file test_field.cpp
 * @brief Unit tests for Fp field arithmetic.
 */

#include <rescue/field.hpp>

#include <gtest/gtest.h>

using namespace rescue;

class FpTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test values
        zero = Fp::ZERO;
        one = Fp::ONE;
        two = Fp(uint64_t{2});
        three = Fp(uint64_t{3});
    }

    Fp zero;
    Fp one;
    Fp two;
    Fp three;
};

TEST_F(FpTest, Constants) {
    EXPECT_TRUE(Fp::ZERO.is_zero());
    EXPECT_TRUE(Fp::ONE.is_one());
    EXPECT_FALSE(Fp::ZERO.is_one());
    EXPECT_FALSE(Fp::ONE.is_zero());
}

TEST_F(FpTest, Construction) {
    // Default construction
    Fp default_fp;
    EXPECT_TRUE(default_fp.is_zero());

    // From uint64_t
    Fp from_uint(uint64_t{42});
    EXPECT_EQ(from_uint.value(), 42);

    // From mpz_class
    Fp from_mpz(mpz_class("12345678901234567890"));
    EXPECT_EQ(from_mpz.value(), mpz_class("12345678901234567890"));

    // From hex string
    Fp from_hex("0x10");
    EXPECT_EQ(from_hex.value(), 16);

    // From hex string without prefix
    Fp from_hex_no_prefix("FF");
    EXPECT_EQ(from_hex_no_prefix.value(), 255);
}

TEST_F(FpTest, Reduction) {
    // Value larger than p should be reduced
    mpz_class large_value = Fp::P + 100;
    Fp reduced(large_value);
    EXPECT_EQ(reduced.value(), 100);

    // Value equal to p should become 0
    Fp at_p(Fp::P);
    EXPECT_TRUE(at_p.is_zero());
}

TEST_F(FpTest, Addition) {
    EXPECT_EQ(zero + zero, zero);
    EXPECT_EQ(zero + one, one);
    EXPECT_EQ(one + one, two);
    EXPECT_EQ(one + two, three);

    // Addition with reduction
    Fp almost_p(Fp::P - 1);
    Fp result = almost_p + one;
    EXPECT_TRUE(result.is_zero());
}

TEST_F(FpTest, Subtraction) {
    EXPECT_EQ(one - zero, one);
    EXPECT_EQ(one - one, zero);
    EXPECT_EQ(three - one, two);
    EXPECT_EQ(two - three, Fp(Fp::P - 1));

    // Subtraction with wrap-around
    Fp result = zero - one;
    EXPECT_EQ(result.value(), Fp::P - 1);
}

TEST_F(FpTest, Multiplication) {
    EXPECT_EQ(zero * one, zero);
    EXPECT_EQ(one * one, one);
    EXPECT_EQ(two * two, Fp(uint64_t{4}));
    EXPECT_EQ(two * three, Fp(uint64_t{6}));
}

TEST_F(FpTest, Negation) {
    EXPECT_EQ(-zero, zero);
    EXPECT_EQ(-one + one, zero);
    EXPECT_EQ(one + (-one), zero);

    Fp neg_two = -two;
    EXPECT_EQ(neg_two + two, zero);
}

TEST_F(FpTest, Inverse) {
    // 1^(-1) = 1
    EXPECT_EQ(one.inv(), one);

    // a * a^(-1) = 1
    EXPECT_EQ(two * two.inv(), one);
    EXPECT_EQ(three * three.inv(), one);

    // Random value test
    Fp random_val = Fp::random();
    if (!random_val.is_zero()) {
        EXPECT_EQ(random_val * random_val.inv(), one);
    }

    // Zero inverse should throw
    EXPECT_THROW(zero.inv(), std::domain_error);
}

TEST_F(FpTest, Power) {
    EXPECT_EQ(two.pow(uint64_t{0}), one);
    EXPECT_EQ(two.pow(uint64_t{1}), two);
    EXPECT_EQ(two.pow(uint64_t{2}), Fp(uint64_t{4}));
    EXPECT_EQ(two.pow(uint64_t{3}), Fp(uint64_t{8}));
    EXPECT_EQ(two.pow(uint64_t{10}), Fp(uint64_t{1024}));

    // Fermat's little theorem: a^(p-1) = 1 for a != 0
    Fp a(uint64_t{7});
    EXPECT_EQ(a.pow(Fp::P - 1), one);
}

TEST_F(FpTest, Square) {
    EXPECT_EQ(zero.square(), zero);
    EXPECT_EQ(one.square(), one);
    EXPECT_EQ(two.square(), Fp(uint64_t{4}));
    EXPECT_EQ(three.square(), Fp(uint64_t{9}));
}

TEST_F(FpTest, Comparison) {
    EXPECT_EQ(one, one);
    EXPECT_NE(one, two);

    EXPECT_LT(one, two);
    EXPECT_GT(two, one);
    EXPECT_LE(one, one);
    EXPECT_GE(two, two);
}

TEST_F(FpTest, Serialization) {
    Fp original(uint64_t{0x123456789ABCDEF0ULL});

    // Serialize
    auto bytes = original.to_bytes();
    EXPECT_EQ(bytes.size(), Fp::BYTES);

    // Deserialize
    Fp deserialized = Fp::from_bytes(bytes);
    EXPECT_EQ(original, deserialized);

    // Test with large value
    Fp large(Fp::P - 1);
    auto large_bytes = large.to_bytes();
    Fp large_back = Fp::from_bytes(large_bytes);
    EXPECT_EQ(large, large_back);
}

TEST_F(FpTest, StringConversion) {
    Fp val(uint64_t{255});

    // Hex string
    std::string hex = val.to_hex();
    EXPECT_TRUE(hex.find("ff") != std::string::npos || hex.find("FF") != std::string::npos);

    // Decimal string
    std::string dec = val.to_string();
    EXPECT_EQ(dec, "255");
}

TEST_F(FpTest, Random) {
    // Generate multiple random values and check they're in range
    for (int i = 0; i < 100; ++i) {
        Fp random_val = Fp::random();
        EXPECT_GE(random_val.value(), 0);
        EXPECT_LT(random_val.value(), Fp::P);
    }

    // Statistical test: random values should not all be the same
    Fp first = Fp::random();
    bool found_different = false;
    for (int i = 0; i < 100; ++i) {
        if (Fp::random() != first) {
            found_different = true;
            break;
        }
    }
    EXPECT_TRUE(found_different);
}

TEST_F(FpTest, OperatorOverloads) {
    Fp a(uint64_t{10});
    Fp b(uint64_t{3});

    // Compound assignment
    Fp c = a;
    c += b;
    EXPECT_EQ(c, Fp(uint64_t{13}));

    c = a;
    c -= b;
    EXPECT_EQ(c, Fp(uint64_t{7}));

    c = a;
    c *= b;
    EXPECT_EQ(c, Fp(uint64_t{30}));
}
