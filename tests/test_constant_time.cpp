/**
 * @file test_constant_time.cpp
 * @brief Unit tests for constant-time operations.
 */

#include <rescue/constant_time.hpp>
#include <rescue/field.hpp>

#include <gtest/gtest.h>

using namespace rescue;
using namespace rescue::ct;

class ConstantTimeTest : public ::testing::Test {
protected:
    void SetUp() override { bin_size = get_bin_size(Fp::P - 1); }

    size_t bin_size;
};

TEST_F(ConstantTimeTest, ToBinLE) {
    // Test zero
    auto zero_bin = to_bin_le(0, 8);
    EXPECT_EQ(zero_bin.size(), 8);
    for (bool bit : zero_bin) {
        EXPECT_FALSE(bit);
    }

    // Test 5 = 101 in binary
    auto five_bin = to_bin_le(5, 8);
    EXPECT_TRUE(five_bin[0]);   // LSB
    EXPECT_FALSE(five_bin[1]);  // 0
    EXPECT_TRUE(five_bin[2]);   // 1
    for (size_t i = 3; i < 8; ++i) {
        EXPECT_FALSE(five_bin[i]);
    }

    // Test negative number (-1 in 2's complement)
    auto neg_one_bin = to_bin_le(-1, 8);
    for (bool bit : neg_one_bin) {
        EXPECT_TRUE(bit);  // All 1s
    }
}

TEST_F(ConstantTimeTest, FromBinLE) {
    // Test zero
    std::vector<bool> zero_bin(8, false);
    EXPECT_EQ(from_bin_le(zero_bin), 0);

    // Test 5
    std::vector<bool> five_bin = {true, false, true, false, false, false, false, false};
    EXPECT_EQ(from_bin_le(five_bin), 5);

    // Test negative: -1 = all 1s in 2's complement
    std::vector<bool> neg_one_bin(8, true);
    EXPECT_EQ(from_bin_le(neg_one_bin), -1);

    // Test roundtrip
    for (long val : {0L, 1L, 5L, 127L, -1L, -5L, -128L}) {
        auto bin = to_bin_le(mpz_class(val), 16);
        auto back = from_bin_le(bin);
        EXPECT_EQ(back, val);
    }
}

TEST_F(ConstantTimeTest, GetBit) {
    // Test bits of 10 (1010 in binary)
    mpz_class ten = 10;
    EXPECT_FALSE(get_bit(ten, 0));  // LSB = 0
    EXPECT_TRUE(get_bit(ten, 1));   // 1
    EXPECT_FALSE(get_bit(ten, 2));  // 0
    EXPECT_TRUE(get_bit(ten, 3));   // 1 (MSB of 10)
    EXPECT_FALSE(get_bit(ten, 4));  // 0 (beyond value)
}

TEST_F(ConstantTimeTest, SignBit) {
    // Positive numbers have sign bit 0
    EXPECT_FALSE(sign_bit(0, 8));
    EXPECT_FALSE(sign_bit(1, 8));
    EXPECT_FALSE(sign_bit(127, 8));

    // Negative numbers have sign bit 1 (in 2's complement)
    EXPECT_TRUE(sign_bit(-1, 8));
    EXPECT_TRUE(sign_bit(-128, 8));
}

TEST_F(ConstantTimeTest, Adder) {
    // 5 + 3 = 8
    auto five_bin = to_bin_le(5, 8);
    auto three_bin = to_bin_le(3, 8);
    auto result = adder(five_bin, three_bin, false, 8);
    EXPECT_EQ(from_bin_le(result), 8);

    // With carry in: 5 + 3 + 1 = 9
    result = adder(five_bin, three_bin, true, 8);
    EXPECT_EQ(from_bin_le(result), 9);

    // Test overflow wrap-around: 255 + 1 = 0 (in 8 bits unsigned, -1 in signed)
    auto max_bin = to_bin_le(255, 8);
    auto one_bin = to_bin_le(1, 8);
    result = adder(max_bin, one_bin, false, 8);
    // In 2's complement signed, this is 0
    EXPECT_EQ(from_bin_le(result), 0);
}

TEST_F(ConstantTimeTest, Add) {
    // Basic addition
    EXPECT_EQ(add(5, 3, 16), 8);
    EXPECT_EQ(add(0, 0, 16), 0);
    EXPECT_EQ(add(100, 200, 16), 300);

    // Addition with field elements
    mpz_class a = Fp::P / 2;
    mpz_class b = 100;
    mpz_class sum = add(a, b, bin_size);
    EXPECT_EQ(sum, a + b);
}

TEST_F(ConstantTimeTest, Sub) {
    // Basic subtraction
    EXPECT_EQ(sub(8, 3, 16), 5);
    EXPECT_EQ(sub(0, 0, 16), 0);
    EXPECT_EQ(sub(300, 200, 16), 100);

    // Subtraction resulting in negative (in 2's complement)
    mpz_class result = sub(3, 8, 16);
    EXPECT_EQ(result, -5);
}

TEST_F(ConstantTimeTest, Lt) {
    // Basic comparisons
    EXPECT_TRUE(lt(3, 8, 16));
    EXPECT_FALSE(lt(8, 3, 16));
    EXPECT_FALSE(lt(5, 5, 16));

    // Edge cases
    EXPECT_TRUE(lt(0, 1, 16));
    EXPECT_FALSE(lt(0, 0, 16));

    // Larger values
    mpz_class a = Fp::P / 3;
    mpz_class b = Fp::P / 2;
    EXPECT_TRUE(lt(a, b, bin_size));
    EXPECT_FALSE(lt(b, a, bin_size));
}

TEST_F(ConstantTimeTest, Select) {
    mpz_class x = 42;
    mpz_class y = 100;

    // Select x when condition is true
    EXPECT_EQ(select(true, x, y, 16), x);

    // Select y when condition is false
    EXPECT_EQ(select(false, x, y, 16), y);
}

TEST_F(ConstantTimeTest, FieldAdd) {
    mpz_class p = Fp::P;

    // Basic field addition
    mpz_class a = 100;
    mpz_class b = 200;
    EXPECT_EQ(field_add(a, b, p, bin_size), 300);

    // Addition with reduction
    mpz_class almost_p = p - 1;
    EXPECT_EQ(field_add(almost_p, 1, p, bin_size), 0);
    EXPECT_EQ(field_add(almost_p, 2, p, bin_size), 1);
}

TEST_F(ConstantTimeTest, FieldSub) {
    mpz_class p = Fp::P;

    // Basic field subtraction
    mpz_class a = 300;
    mpz_class b = 100;
    EXPECT_EQ(field_sub(a, b, p, bin_size), 200);

    // Subtraction with wrap-around
    EXPECT_EQ(field_sub(0, 1, p, bin_size), p - 1);
    EXPECT_EQ(field_sub(1, 2, p, bin_size), p - 1);
}

TEST_F(ConstantTimeTest, VerifyBinSize) {
    // Values that fit
    EXPECT_TRUE(verify_bin_size(0, 8));
    EXPECT_TRUE(verify_bin_size(127, 8));
    EXPECT_TRUE(verify_bin_size(-128, 8));
    EXPECT_TRUE(verify_bin_size(-1, 8));

    // Values that don't fit
    EXPECT_FALSE(verify_bin_size(256, 8));
    EXPECT_FALSE(verify_bin_size(-256, 8));
}

TEST_F(ConstantTimeTest, GetBinSize) {
    // Small values
    EXPECT_GE(get_bin_size(255), 8 + 3);

    // Field modulus
    size_t fp_bin_size = get_bin_size(Fp::P - 1);
    EXPECT_GE(fp_bin_size, 255 + 3);
}

TEST_F(ConstantTimeTest, ConsistencyWithRegularArithmetic) {
    // Verify CT operations match regular operations for field elements
    for (int i = 0; i < 100; ++i) {
        Fp a = Fp::random();
        Fp b = Fp::random();

        // Field addition
        Fp regular_sum = a + b;
        mpz_class ct_sum = field_add(a.value(), b.value(), Fp::P, bin_size);
        EXPECT_EQ(regular_sum.value(), ct_sum);

        // Field subtraction
        Fp regular_diff = a - b;
        mpz_class ct_diff = field_sub(a.value(), b.value(), Fp::P, bin_size);
        EXPECT_EQ(regular_diff.value(), ct_diff);
    }
}
