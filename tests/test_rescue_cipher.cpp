/**
 * @file test_rescue_cipher.cpp
 * @brief Unit tests for Rescue cipher (CTR mode).
 */

#include <rescue/rescue_cipher.hpp>
#include <rescue/utils.hpp>

#include <gtest/gtest.h>

using namespace rescue;

class RescueCipherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test shared secret
        shared_secret = random_bytes<RESCUE_CIPHER_SECRET_SIZE>();

        // Create a test nonce
        nonce = generate_nonce();

        // Create cipher instance
        cipher = std::make_unique<RescueCipher>(shared_secret);
    }

    std::array<uint8_t, RESCUE_CIPHER_SECRET_SIZE> shared_secret;
    std::array<uint8_t, RESCUE_CIPHER_NONCE_SIZE> nonce;
    std::unique_ptr<RescueCipher> cipher;
};

TEST_F(RescueCipherTest, Construction) {
    // From array
    EXPECT_NO_THROW((RescueCipher(shared_secret)));

    // From vector
    std::vector<uint8_t> secret_vec(shared_secret.begin(), shared_secret.end());
    EXPECT_NO_THROW((RescueCipher(secret_vec)));

    // Invalid size should throw
    std::vector<uint8_t> short_secret(16, 0);
    EXPECT_THROW((RescueCipher(short_secret)), std::invalid_argument);

    std::vector<uint8_t> long_secret(64, 0);
    EXPECT_THROW((RescueCipher(long_secret)), std::invalid_argument);
}

TEST_F(RescueCipherTest, EncryptDecryptRoundtrip) {
    std::vector<Fp> plaintext = {
        Fp(uint64_t{42}),
        Fp(uint64_t{1337}),
        Fp(uint64_t{0xDEADBEEF})
    };

    // Encrypt
    auto ciphertext = cipher->encrypt(plaintext, nonce);
    EXPECT_EQ(ciphertext.size(), plaintext.size());

    // Decrypt
    auto decrypted = cipher->decrypt(ciphertext, nonce);
    EXPECT_EQ(decrypted.size(), plaintext.size());

    // Verify roundtrip
    for (size_t i = 0; i < plaintext.size(); ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i])
            << "Mismatch at index " << i;
    }
}

TEST_F(RescueCipherTest, RawEncryptDecryptRoundtrip) {
    std::vector<Fp> plaintext = {Fp(uint64_t{100}), Fp(uint64_t{200})};

    // Raw encrypt
    auto ciphertext = cipher->encrypt_raw(plaintext, nonce);
    EXPECT_EQ(ciphertext.size(), plaintext.size());

    // Ciphertext should differ from plaintext
    bool any_different = false;
    for (size_t i = 0; i < plaintext.size(); ++i) {
        if (ciphertext[i] != plaintext[i]) {
            any_different = true;
            break;
        }
    }
    EXPECT_TRUE(any_different);

    // Raw decrypt
    auto decrypted = cipher->decrypt_raw(ciphertext, nonce);
    EXPECT_EQ(decrypted.size(), plaintext.size());

    for (size_t i = 0; i < plaintext.size(); ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(RescueCipherTest, EmptyPlaintext) {
    std::vector<Fp> empty_plaintext;

    auto ciphertext = cipher->encrypt(empty_plaintext, nonce);
    EXPECT_TRUE(ciphertext.empty());

    auto decrypted = cipher->decrypt(ciphertext, nonce);
    EXPECT_TRUE(decrypted.empty());
}

TEST_F(RescueCipherTest, SingleElement) {
    std::vector<Fp> plaintext = {Fp(uint64_t{42})};

    auto ciphertext = cipher->encrypt(plaintext, nonce);
    EXPECT_EQ(ciphertext.size(), 1);

    auto decrypted = cipher->decrypt(ciphertext, nonce);
    EXPECT_EQ(decrypted[0], plaintext[0]);
}

TEST_F(RescueCipherTest, ExactBlockSize) {
    // Exactly one block (5 elements)
    std::vector<Fp> plaintext;
    for (size_t i = 0; i < RESCUE_CIPHER_BLOCK_SIZE; ++i) {
        plaintext.push_back(Fp(static_cast<uint64_t>(i + 1)));
    }

    auto ciphertext = cipher->encrypt(plaintext, nonce);
    auto decrypted = cipher->decrypt(ciphertext, nonce);

    EXPECT_EQ(decrypted.size(), plaintext.size());
    for (size_t i = 0; i < plaintext.size(); ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(RescueCipherTest, MultipleBlocks) {
    // Multiple blocks
    std::vector<Fp> plaintext;
    for (size_t i = 0; i < 3 * RESCUE_CIPHER_BLOCK_SIZE + 2; ++i) {
        plaintext.push_back(Fp(static_cast<uint64_t>(i)));
    }

    auto ciphertext = cipher->encrypt(plaintext, nonce);
    auto decrypted = cipher->decrypt(ciphertext, nonce);

    EXPECT_EQ(decrypted.size(), plaintext.size());
    for (size_t i = 0; i < plaintext.size(); ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(RescueCipherTest, DifferentNoncesProduceDifferentCiphertext) {
    std::vector<Fp> plaintext = {Fp(uint64_t{42})};

    auto nonce1 = generate_nonce();
    auto nonce2 = generate_nonce();

    auto ct1 = cipher->encrypt_raw(plaintext, nonce1);
    auto ct2 = cipher->encrypt_raw(plaintext, nonce2);

    // Same plaintext with different nonces should produce different ciphertext
    EXPECT_NE(ct1[0], ct2[0]);
}

TEST_F(RescueCipherTest, SameNonceProducesSameCiphertext) {
    std::vector<Fp> plaintext = {Fp(uint64_t{42})};

    auto ct1 = cipher->encrypt_raw(plaintext, nonce);
    auto ct2 = cipher->encrypt_raw(plaintext, nonce);

    // Same plaintext with same nonce should produce same ciphertext
    EXPECT_EQ(ct1[0], ct2[0]);
}

TEST_F(RescueCipherTest, DifferentKeysProduceDifferentCiphertext) {
    std::vector<Fp> plaintext = {Fp(uint64_t{42})};

    auto secret1 = random_bytes<RESCUE_CIPHER_SECRET_SIZE>();
    auto secret2 = random_bytes<RESCUE_CIPHER_SECRET_SIZE>();

    RescueCipher cipher1(secret1);
    RescueCipher cipher2(secret2);

    auto ct1 = cipher1.encrypt_raw(plaintext, nonce);
    auto ct2 = cipher2.encrypt_raw(plaintext, nonce);

    // Same plaintext with different keys should produce different ciphertext
    EXPECT_NE(ct1[0], ct2[0]);
}

TEST_F(RescueCipherTest, WrongKeyDecryptsFails) {
    std::vector<Fp> plaintext = {Fp(uint64_t{42})};

    // Encrypt with one key
    auto ciphertext = cipher->encrypt(plaintext, nonce);

    // Try to decrypt with different key
    auto different_secret = random_bytes<RESCUE_CIPHER_SECRET_SIZE>();
    RescueCipher different_cipher(different_secret);
    auto decrypted = different_cipher.decrypt(ciphertext, nonce);

    // Should not match original plaintext
    EXPECT_NE(decrypted[0], plaintext[0]);
}

TEST_F(RescueCipherTest, SerializedFormat) {
    std::vector<Fp> plaintext = {Fp(uint64_t{42})};

    auto ciphertext = cipher->encrypt(plaintext, nonce);

    // Each element should be 32 bytes
    EXPECT_EQ(ciphertext[0].size(), Fp::BYTES);
}

TEST_F(RescueCipherTest, InvalidNonceSize) {
    std::vector<Fp> plaintext = {Fp(uint64_t{42})};
    std::vector<uint8_t> short_nonce(8, 0);
    std::vector<uint8_t> long_nonce(32, 0);

    EXPECT_THROW(cipher->encrypt(plaintext, short_nonce), std::invalid_argument);
    EXPECT_THROW(cipher->encrypt(plaintext, long_nonce), std::invalid_argument);
}

TEST_F(RescueCipherTest, InvalidCiphertextFormat) {
    std::vector<std::vector<uint8_t>> bad_ciphertext = {{0, 1, 2}};  // Not 32 bytes

    EXPECT_THROW(cipher->decrypt(bad_ciphertext, nonce), std::invalid_argument);
}

TEST_F(RescueCipherTest, LargeValues) {
    // Test with values close to field modulus
    std::vector<Fp> plaintext = {
        Fp(Fp::P - uint256::one()),  // Maximum valid value
        Fp(Fp::P - uint256{2}),
        Fp(Fp::P >> 1)               // Fp::P / 2
    };

    auto ciphertext = cipher->encrypt(plaintext, nonce);
    auto decrypted = cipher->decrypt(ciphertext, nonce);

    for (size_t i = 0; i < plaintext.size(); ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(RescueCipherTest, ZeroValues) {
    std::vector<Fp> plaintext = {Fp::ZERO, Fp::ZERO, Fp::ZERO};

    auto ciphertext = cipher->encrypt(plaintext, nonce);
    auto decrypted = cipher->decrypt(ciphertext, nonce);

    for (size_t i = 0; i < plaintext.size(); ++i) {
        EXPECT_EQ(decrypted[i], plaintext[i]);
    }
}

TEST_F(RescueCipherTest, RandomPlaintext) {
    // Test with random plaintext
    for (int trial = 0; trial < 10; ++trial) {
        std::vector<Fp> plaintext;
        size_t len = (trial % 10) + 1;
        for (size_t i = 0; i < len; ++i) {
            plaintext.push_back(Fp::random());
        }

        auto test_nonce = generate_nonce();
        auto ciphertext = cipher->encrypt(plaintext, test_nonce);
        auto decrypted = cipher->decrypt(ciphertext, test_nonce);

        EXPECT_EQ(decrypted.size(), plaintext.size());
        for (size_t i = 0; i < plaintext.size(); ++i) {
            EXPECT_EQ(decrypted[i], plaintext[i])
                << "Trial " << trial << ", index " << i;
        }
    }
}

TEST_F(RescueCipherTest, GenerateNonce) {
    // Generate multiple nonces and verify they're different
    auto nonce1 = generate_nonce();
    auto nonce2 = generate_nonce();
    auto nonce3 = generate_nonce();

    EXPECT_NE(nonce1, nonce2);
    EXPECT_NE(nonce2, nonce3);
    EXPECT_NE(nonce1, nonce3);

    // Verify nonce size
    EXPECT_EQ(nonce1.size(), RESCUE_CIPHER_NONCE_SIZE);
}

TEST_F(RescueCipherTest, CTRModeProperty) {
    // CTR mode: encrypting same block twice with same counter should give same result
    std::vector<Fp> plaintext1 = {Fp(uint64_t{100})};
    std::vector<Fp> plaintext2 = {Fp(uint64_t{100})};

    auto ct1 = cipher->encrypt_raw(plaintext1, nonce);
    auto ct2 = cipher->encrypt_raw(plaintext2, nonce);

    EXPECT_EQ(ct1, ct2);
}
