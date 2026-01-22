#include <rescue/utils.hpp>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <cstring>
#include <stdexcept>

namespace rescue {

std::vector<uint8_t> serialize_le(const mpz_class& value, size_t length_in_bytes) {
    std::vector<uint8_t> result(length_in_bytes, 0);
    mpz_class temp = value;

    for (size_t i = 0; i < length_in_bytes; ++i) {
        result[i] = static_cast<uint8_t>(mpz_get_ui(temp.get_mpz_t()) & 0xFF);
        temp >>= 8;
    }

    if (temp > 0) {
        throw std::overflow_error("Value is too large for the specified byte length");
    }

    return result;
}

mpz_class deserialize_le(std::span<const uint8_t> bytes) {
    mpz_class result = 0;
    for (size_t i = bytes.size(); i > 0; --i) {
        result <<= 8;
        result |= bytes[i - 1];
    }
    return result;
}

std::vector<uint8_t> random_bytes(size_t length) {
    std::vector<uint8_t> result(length);
    if (RAND_bytes(result.data(), static_cast<int>(length)) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }
    return result;
}

mpz_class random_field_elem(const mpz_class& bound) {
    // Calculate byte length needed
    size_t byte_length = (mpz_sizeinbase(bound.get_mpz_t(), 2) + 7) / 8;

    mpz_class result;
    do {
        auto bytes = random_bytes(byte_length);
        result = deserialize_le(bytes);
    } while (result >= bound);

    return result;
}

// ============================================================================
// SHAKE256 Implementation
// ============================================================================

Shake256::Shake256() : ctx_(nullptr), finalized_(false) {
    ctx_ = EVP_MD_CTX_new();
    if (!ctx_) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(static_cast<EVP_MD_CTX*>(ctx_), EVP_shake256(), nullptr) != 1) {
        EVP_MD_CTX_free(static_cast<EVP_MD_CTX*>(ctx_));
        throw std::runtime_error("Failed to initialize SHAKE256");
    }
}

Shake256::~Shake256() {
    if (ctx_) {
        EVP_MD_CTX_free(static_cast<EVP_MD_CTX*>(ctx_));
    }
}

Shake256::Shake256(Shake256&& other) noexcept : ctx_(other.ctx_), finalized_(other.finalized_) {
    other.ctx_ = nullptr;
    other.finalized_ = false;
}

Shake256& Shake256::operator=(Shake256&& other) noexcept {
    if (this != &other) {
        if (ctx_) {
            EVP_MD_CTX_free(static_cast<EVP_MD_CTX*>(ctx_));
        }
        ctx_ = other.ctx_;
        finalized_ = other.finalized_;
        other.ctx_ = nullptr;
        other.finalized_ = false;
    }
    return *this;
}

void Shake256::update(std::span<const uint8_t> data) {
    if (finalized_) {
        throw std::logic_error("Cannot update after finalization");
    }
    if (EVP_DigestUpdate(static_cast<EVP_MD_CTX*>(ctx_), data.data(), data.size()) != 1) {
        throw std::runtime_error("Failed to update SHAKE256");
    }
}

void Shake256::update(std::string_view str) {
    update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(str.data()), str.size()));
}

std::vector<uint8_t> Shake256::xof(size_t length) {
    std::vector<uint8_t> result(length);

    if (!finalized_) {
        // First squeeze - finalize the absorb phase
        if (EVP_DigestFinalXOF(static_cast<EVP_MD_CTX*>(ctx_), result.data(), length) != 1) {
            throw std::runtime_error("Failed to squeeze SHAKE256");
        }
        finalized_ = true;
    } else {
        // Subsequent squeeze - we need to re-initialize for OpenSSL
        // Note: OpenSSL doesn't support incremental XOF squeezing directly,
        // so we need a workaround for production use
        throw std::logic_error("Multiple xof() calls not supported in this implementation");
    }

    return result;
}

std::vector<uint8_t> Shake256::finalize(size_t length) {
    return xof(length);
}

std::vector<uint8_t> shake256(std::span<const uint8_t> data, size_t output_length) {
    Shake256 hasher;
    hasher.update(data);
    return hasher.finalize(output_length);
}

std::vector<uint8_t> shake256(std::string_view str, size_t output_length) {
    Shake256 hasher;
    hasher.update(str);
    return hasher.finalize(output_length);
}

// ============================================================================
// SHA-256 Implementation
// ============================================================================

std::array<uint8_t, 32> sha256(std::span<const uint8_t> data) {
    std::array<uint8_t, 32> result;
    SHA256(data.data(), data.size(), result.data());
    return result;
}

std::array<uint8_t, 32> sha256(const std::vector<std::span<const uint8_t>>& chunks) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    for (const auto& chunk : chunks) {
        SHA256_Update(&ctx, chunk.data(), chunk.size());
    }

    std::array<uint8_t, 32> result;
    SHA256_Final(result.data(), &ctx);
    return result;
}

}  // namespace rescue
