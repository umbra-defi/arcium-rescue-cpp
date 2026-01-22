#include <rescue/field.hpp>

#include <random>
#include <sstream>
#include <stdexcept>

namespace rescue {

// Static constants initialization
// p = 2^255 - 19
const mpz_class Fp::P("0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed");
const Fp Fp::ZERO(static_cast<unsigned long>(0));
const Fp Fp::ONE(static_cast<unsigned long>(1));

Fp::Fp(uint64_t value) : value_(static_cast<unsigned long>(value)) {
    reduce();
}

Fp::Fp(const mpz_class& value) : value_(value) {
    reduce();
}

Fp::Fp(mpz_class&& value) : value_(std::move(value)) {
    reduce();
}

Fp::Fp(std::string_view hex_str) {
    std::string str(hex_str);
    // Handle 0x prefix
    if (str.size() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str = str.substr(2);
    }
    value_ = mpz_class(str, 16);
    reduce();
}

Fp::Fp(std::span<const uint8_t> bytes) {
    // Little-endian deserialization
    value_ = 0;
    for (size_t i = bytes.size(); i > 0; --i) {
        value_ <<= 8;
        value_ |= bytes[i - 1];
    }
    reduce();
}

void Fp::reduce() {
    // Ensure value is in [0, p)
    if (value_ < 0) {
        value_ = ((value_ % P) + P) % P;
    } else if (value_ >= P) {
        value_ %= P;
    }
}

Fp Fp::random() {
    // Generate random bytes and reduce
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());

    // Generate 32 random bytes
    std::array<uint8_t, 32> bytes;
    for (size_t i = 0; i < 32; i += sizeof(uint64_t)) {
        uint64_t val = gen();
        for (size_t j = 0; j < sizeof(uint64_t) && (i + j) < 32; ++j) {
            bytes[i + j] = static_cast<uint8_t>((val >> (j * 8)) & 0xFF);
        }
    }

    // Convert bytes to mpz_class (little-endian)
    mpz_class result = 0;
    for (size_t i = 32; i > 0; --i) {
        result <<= 8;
        result |= static_cast<unsigned long>(bytes[i - 1]);
    }
    // Reduce modulo p
    result %= P;
    return Fp(std::move(result));
}

Fp Fp::create(const mpz_class& value) {
    return Fp(value);
}

Fp Fp::add(const Fp& rhs) const {
    mpz_class result = value_ + rhs.value_;
    if (result >= P) {
        result -= P;
    }
    return Fp(std::move(result));
}

Fp Fp::sub(const Fp& rhs) const {
    mpz_class result = value_ - rhs.value_;
    if (result < 0) {
        result += P;
    }
    return Fp(std::move(result));
}

Fp Fp::mul(const Fp& rhs) const {
    mpz_class result;
    mpz_mul(result.get_mpz_t(), value_.get_mpz_t(), rhs.value_.get_mpz_t());
    mpz_mod(result.get_mpz_t(), result.get_mpz_t(), P.get_mpz_t());
    return Fp(std::move(result));
}

Fp Fp::neg() const {
    if (value_ == 0) {
        return *this;
    }
    return Fp(P - value_);
}

Fp Fp::inv() const {
    if (is_zero()) {
        throw std::domain_error("Cannot invert zero in field");
    }
    mpz_class result;
    if (mpz_invert(result.get_mpz_t(), value_.get_mpz_t(), P.get_mpz_t()) == 0) {
        throw std::domain_error("Failed to compute inverse");
    }
    return Fp(std::move(result));
}

Fp Fp::pow(const mpz_class& exp) const {
    if (exp < 0) {
        // Negative exponent: compute inverse first
        return inv().pow(-exp);
    }

    mpz_class result;
    mpz_powm(result.get_mpz_t(), value_.get_mpz_t(), exp.get_mpz_t(), P.get_mpz_t());
    return Fp(std::move(result));
}

Fp Fp::pow(uint64_t exp) const {
    return pow(mpz_class(static_cast<unsigned long>(exp)));
}

Fp Fp::square() const {
    mpz_class result;
    mpz_mul(result.get_mpz_t(), value_.get_mpz_t(), value_.get_mpz_t());
    mpz_mod(result.get_mpz_t(), result.get_mpz_t(), P.get_mpz_t());
    return Fp(std::move(result));
}

bool Fp::is_zero() const {
    return value_ == 0;
}

bool Fp::is_one() const {
    return value_ == 1;
}

bool Fp::operator==(const Fp& rhs) const {
    return value_ == rhs.value_;
}

std::strong_ordering Fp::operator<=>(const Fp& rhs) const {
    int cmp = mpz_cmp(value_.get_mpz_t(), rhs.value_.get_mpz_t());
    if (cmp < 0) return std::strong_ordering::less;
    if (cmp > 0) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

std::array<uint8_t, Fp::BYTES> Fp::to_bytes() const {
    std::array<uint8_t, BYTES> result{};
    to_bytes(result);
    return result;
}

void Fp::to_bytes(std::span<uint8_t, BYTES> out) const {
    // Little-endian serialization
    mpz_class temp = value_;
    for (size_t i = 0; i < BYTES; ++i) {
        out[i] = static_cast<uint8_t>(mpz_get_ui(temp.get_mpz_t()) & 0xFF);
        temp >>= 8;
    }
}

Fp Fp::from_bytes(std::span<const uint8_t> bytes) {
    return Fp(bytes);
}

std::string Fp::to_hex() const {
    std::ostringstream oss;
    oss << "0x" << std::hex << value_;
    return oss.str();
}

std::string Fp::to_string() const {
    return value_.get_str();
}

std::ostream& operator<<(std::ostream& os, const Fp& fp) {
    return os << fp.to_string();
}

mpz_class mod_inverse(const mpz_class& a, const mpz_class& m) {
    mpz_class result;
    if (mpz_invert(result.get_mpz_t(), a.get_mpz_t(), m.get_mpz_t()) == 0) {
        throw std::domain_error("No modular inverse exists");
    }
    return result;
}

}  // namespace rescue
