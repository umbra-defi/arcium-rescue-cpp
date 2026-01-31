#include <rescue/field.hpp>

#include <random>
#include <sstream>
#include <stdexcept>

// For OpenSSL random bytes
#include <openssl/rand.h>

namespace rescue {

// Static constants initialization
// p = 2^255 - 19
const uint256 Fp::P = fp::P;
const Fp Fp::ZERO{uint64_t{0}};
const Fp Fp::ONE{uint64_t{1}};

Fp::Fp(uint64_t value) : value_(value) {
    // Small values don't need reduction
    // but we call reduce() for consistency
    if (value >= 0x7fffffffffffffffULL) {
        reduce();
    }
}

Fp::Fp(const uint256& value) : value_(value) {
    reduce();
}

Fp::Fp(uint256&& value) : value_(std::move(value)) {
    reduce();
}

Fp::Fp(std::string_view hex_str) : value_(hex_str) {
    reduce();
}

Fp::Fp(std::span<const uint8_t> bytes) : value_(bytes) {
    reduce();
}

void Fp::reduce() {
    // Ensure value is in [0, p)
    value_ = fp::reduce_full(value_);
}

Fp Fp::random() {
    // Generate 32 random bytes using OpenSSL
    std::array<uint8_t, 32> bytes;
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1) {
        // Fallback to std::random_device if OpenSSL fails
        std::random_device rd;
        std::mt19937_64 gen(rd());
        for (size_t i = 0; i < 32; i += sizeof(uint64_t)) {
            uint64_t val = gen();
            for (size_t j = 0; j < sizeof(uint64_t) && (i + j) < 32; ++j) {
                bytes[i + j] = static_cast<uint8_t>((val >> (j * 8)) & 0xFF);
            }
        }
    }

    // Convert bytes to uint256 (little-endian)
    uint256 result = uint256::from_bytes(bytes);

    // Reduce modulo p
    result = fp::reduce_full(result);

    return Fp(std::move(result));
}

Fp Fp::create(const uint256& value) {
    return Fp(value);
}

Fp Fp::add(const Fp& rhs) const {
    return Fp(fp::add(value_, rhs.value_));
}

Fp Fp::sub(const Fp& rhs) const {
    return Fp(fp::sub(value_, rhs.value_));
}

Fp Fp::mul(const Fp& rhs) const {
    return Fp(fp::mul(value_, rhs.value_));
}

Fp Fp::neg() const {
    // fp::neg already handles zero correctly via constant-time mask
    // No branch needed here
    return Fp(fp::neg(value_));
}

Fp Fp::inv() const {
    if (is_zero()) {
        throw std::domain_error("Cannot invert zero in field");
    }
    return Fp(fp::inv(value_));
}

Fp Fp::pow(const uint256& exp) const {
    return Fp(fp::pow(value_, exp));
}

Fp Fp::pow(uint64_t exp) const {
    return Fp(fp::pow(value_, exp));
}

Fp Fp::square() const {
    return Fp(fp::sqr(value_));
}

bool Fp::is_zero() const {
    return value_.is_zero();
}

bool Fp::is_one() const {
    return value_.is_one();
}

bool Fp::operator==(const Fp& rhs) const {
    return value_ == rhs.value_;
}

std::strong_ordering Fp::operator<=>(const Fp& rhs) const {
    return value_ <=> rhs.value_;
}

std::array<uint8_t, Fp::BYTES> Fp::to_bytes() const {
    return value_.to_bytes_le();
}

void Fp::to_bytes(std::span<uint8_t, BYTES> out) const {
    value_.to_bytes_le(out);
}

Fp Fp::from_bytes(std::span<const uint8_t> bytes) {
    return Fp(bytes);
}

std::string Fp::to_hex() const {
    return value_.to_hex();
}

std::string Fp::to_string() const {
    return value_.to_string();
}

std::ostream& operator<<(std::ostream& os, const Fp& fp) {
    return os << fp.to_string();
}

uint256 mod_inverse(const uint256& a, const uint256& /*m*/) {
    // We always use the optimized inversion for p = 2^255 - 19
    return fp::inv(a);
}

}  // namespace rescue
