/**
 * @file test_rescue_hash.cpp
 * @brief Unit tests for Rescue-Prime hash function.
 */

#include <rescue/rescue_hash.hpp>

#include <gtest/gtest.h>

using namespace rescue;

class RescueHashTest : public ::testing::Test {
protected:
    void SetUp() override { hasher = std::make_unique<RescuePrimeHash>(); }

    std::unique_ptr<RescuePrimeHash> hasher;
};

TEST_F(RescueHashTest, DefaultConstruction) {
    EXPECT_EQ(hasher->rate(), RESCUE_HASH_RATE);
    EXPECT_EQ(hasher->capacity(), RESCUE_HASH_CAPACITY);
    EXPECT_EQ(hasher->digest_length(), RESCUE_HASH_DIGEST_LENGTH);
    EXPECT_EQ(hasher->state_size(), RESCUE_HASH_STATE_SIZE);
}

TEST_F(RescueHashTest, CustomConstruction) {
    RescuePrimeHash custom(5, 3, 3);  // rate=5, capacity=3, digest=3
    EXPECT_EQ(custom.rate(), 5);
    EXPECT_EQ(custom.capacity(), 3);
    EXPECT_EQ(custom.digest_length(), 3);
    EXPECT_EQ(custom.state_size(), 8);
}

TEST_F(RescueHashTest, InvalidConstruction) {
    // Zero rate
    EXPECT_THROW(RescuePrimeHash(0, 5, 5), std::invalid_argument);

    // Zero capacity
    EXPECT_THROW(RescuePrimeHash(5, 0, 5), std::invalid_argument);

    // Zero digest length
    EXPECT_THROW(RescuePrimeHash(5, 5, 0), std::invalid_argument);

    // Digest length > state size
    EXPECT_THROW(RescuePrimeHash(5, 5, 11), std::invalid_argument);
}

TEST_F(RescueHashTest, EmptyMessage) {
    std::vector<Fp> empty_msg;
    auto digest = hasher->digest(empty_msg);

    EXPECT_EQ(digest.size(), RESCUE_HASH_DIGEST_LENGTH);

    // Digest should be deterministic
    auto digest2 = hasher->digest(empty_msg);
    EXPECT_EQ(digest, digest2);
}

TEST_F(RescueHashTest, SingleElement) {
    std::vector<Fp> msg = {Fp(uint64_t{42})};
    auto digest = hasher->digest(msg);

    EXPECT_EQ(digest.size(), RESCUE_HASH_DIGEST_LENGTH);

    // Different input should produce different output
    std::vector<Fp> msg2 = {Fp(uint64_t{43})};
    auto digest2 = hasher->digest(msg2);
    EXPECT_NE(digest, digest2);
}

TEST_F(RescueHashTest, MultipleElements) {
    std::vector<Fp> msg = {
        Fp(uint64_t{1}),
        Fp(uint64_t{2}),
        Fp(uint64_t{3}),
        Fp(uint64_t{4}),
        Fp(uint64_t{5})
    };
    auto digest = hasher->digest(msg);

    EXPECT_EQ(digest.size(), RESCUE_HASH_DIGEST_LENGTH);
}

TEST_F(RescueHashTest, LongMessage) {
    // Message longer than rate
    std::vector<Fp> msg;
    for (size_t i = 0; i < 20; ++i) {
        msg.push_back(Fp(static_cast<uint64_t>(i)));
    }

    auto digest = hasher->digest(msg);
    EXPECT_EQ(digest.size(), RESCUE_HASH_DIGEST_LENGTH);
}

TEST_F(RescueHashTest, Determinism) {
    std::vector<Fp> msg = {
        Fp(uint64_t{100}),
        Fp(uint64_t{200}),
        Fp(uint64_t{300})
    };

    // Hash same message multiple times
    auto digest1 = hasher->digest(msg);
    auto digest2 = hasher->digest(msg);
    auto digest3 = hasher->digest(msg);

    EXPECT_EQ(digest1, digest2);
    EXPECT_EQ(digest2, digest3);
}

TEST_F(RescueHashTest, DifferentInstances) {
    std::vector<Fp> msg = {Fp(uint64_t{12345})};

    // Different hasher instances should produce same result
    RescuePrimeHash hasher1;
    RescuePrimeHash hasher2;

    auto digest1 = hasher1.digest(msg);
    auto digest2 = hasher2.digest(msg);

    EXPECT_EQ(digest1, digest2);
}

TEST_F(RescueHashTest, CollisionResistance) {
    // Generate many random messages and check for collisions
    std::set<std::vector<Fp>> digests;

    for (int i = 0; i < 100; ++i) {
        std::vector<Fp> msg = {Fp::random(), Fp::random()};
        auto digest = hasher->digest(msg);

        // Check we haven't seen this digest before
        EXPECT_TRUE(digests.insert(digest).second)
            << "Collision found at iteration " << i;
    }
}

TEST_F(RescueHashTest, AvalancheEffect) {
    // Small change in input should cause large change in output
    std::vector<Fp> msg1 = {Fp(uint64_t{1000})};
    std::vector<Fp> msg2 = {Fp(uint64_t{1001})};

    auto digest1 = hasher->digest(msg1);
    auto digest2 = hasher->digest(msg2);

    // Count how many elements differ
    int differences = 0;
    for (size_t i = 0; i < digest1.size(); ++i) {
        if (digest1[i] != digest2[i]) {
            differences++;
        }
    }

    // Should have significant differences (avalanche effect)
    EXPECT_GE(differences, 1);
}

TEST_F(RescueHashTest, DigestFromBigInts) {
    std::vector<mpz_class> msg = {
        mpz_class("123456789012345678901234567890"),
        mpz_class("987654321098765432109876543210")
    };

    auto digest = hasher->digest(msg);
    EXPECT_EQ(digest.size(), RESCUE_HASH_DIGEST_LENGTH);
}

TEST_F(RescueHashTest, PaddingCorrectness) {
    // Messages of different lengths that would pad to same length
    // should still produce different outputs

    // Length 6 (pads to 7 with '1', already multiple of rate=7)
    std::vector<Fp> msg1(6, Fp(uint64_t{1}));

    // Length 7 (pads to 14: append '1' then 6 zeros)
    std::vector<Fp> msg2(7, Fp(uint64_t{1}));

    auto digest1 = hasher->digest(msg1);
    auto digest2 = hasher->digest(msg2);

    EXPECT_NE(digest1, digest2);
}

TEST_F(RescueHashTest, OutputInFieldRange) {
    std::vector<Fp> msg = {Fp::random(), Fp::random(), Fp::random()};
    auto digest = hasher->digest(msg);

    // All output elements should be valid field elements
    for (const auto& elem : digest) {
        EXPECT_GE(elem.value(), 0);
        EXPECT_LT(elem.value(), Fp::P);
    }
}

TEST_F(RescueHashTest, ExactRateSizeMessage) {
    // Message exactly rate size
    std::vector<Fp> msg;
    for (size_t i = 0; i < RESCUE_HASH_RATE; ++i) {
        msg.push_back(Fp(static_cast<uint64_t>(i)));
    }

    auto digest = hasher->digest(msg);
    EXPECT_EQ(digest.size(), RESCUE_HASH_DIGEST_LENGTH);
}

TEST_F(RescueHashTest, MultipleOfRateSizeMessage) {
    // Message exactly 2 * rate size
    std::vector<Fp> msg;
    for (size_t i = 0; i < 2 * RESCUE_HASH_RATE; ++i) {
        msg.push_back(Fp(static_cast<uint64_t>(i)));
    }

    auto digest = hasher->digest(msg);
    EXPECT_EQ(digest.size(), RESCUE_HASH_DIGEST_LENGTH);
}
