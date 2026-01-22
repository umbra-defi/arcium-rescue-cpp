#include <rescue/constant_time.hpp>

#include <cmath>
#include <stdexcept>

namespace rescue::ct {

std::vector<bool> to_bin_le(const mpz_class& x, size_t bin_size) {
    std::vector<bool> result;
    result.reserve(bin_size);

    for (size_t i = 0; i < bin_size; ++i) {
        result.push_back(get_bit(x, i));
    }

    return result;
}

mpz_class from_bin_le(const std::vector<bool>& x_bin) {
    if (x_bin.empty()) {
        return 0;
    }

    mpz_class result = 0;

    // Sum up all bits except the sign bit
    for (size_t i = 0; i < x_bin.size() - 1; ++i) {
        if (x_bin[i]) {
            mpz_class bit_val;
            mpz_ui_pow_ui(bit_val.get_mpz_t(), 2, static_cast<unsigned long>(i));
            result += bit_val;
        }
    }

    // Handle sign bit (2's complement)
    // If sign bit is set, subtract 2^(bin_size-1)
    if (x_bin[x_bin.size() - 1]) {
        mpz_class sign_val;
        mpz_ui_pow_ui(sign_val.get_mpz_t(), 2, static_cast<unsigned long>(x_bin.size() - 1));
        result -= sign_val;
    }

    return result;
}

bool get_bit(const mpz_class& x, size_t bit_pos) {
    return mpz_tstbit(x.get_mpz_t(), static_cast<mp_bitcnt_t>(bit_pos)) != 0;
}

bool sign_bit(const mpz_class& x, size_t bin_size) {
    return get_bit(x, bin_size);
}

std::vector<bool> adder(const std::vector<bool>& x_bin,
                        const std::vector<bool>& y_bin,
                        bool carry_in,
                        size_t bin_size) {
    std::vector<bool> result;
    result.reserve(bin_size);

    bool carry = carry_in;

    for (size_t i = 0; i < bin_size; ++i) {
        // result[i] = x_bin[i] XOR y_bin[i] XOR carry
        bool y_xor_carry = (y_bin[i] != carry);
        result.push_back(x_bin[i] != y_xor_carry);

        // new_carry = (x_bin[i] AND y_bin[i]) XOR (x_bin[i] AND carry) XOR (y_bin[i] AND carry)
        //           = (y_bin[i] XOR carry) ? x_bin[i] : y_bin[i]
        bool new_carry = y_bin[i] != (y_xor_carry && (x_bin[i] != y_bin[i]));
        carry = new_carry;
    }

    return result;
}

mpz_class add(const mpz_class& x, const mpz_class& y, size_t bin_size) {
    auto x_bin = to_bin_le(x, bin_size);
    auto y_bin = to_bin_le(y, bin_size);
    auto result_bin = adder(x_bin, y_bin, false, bin_size);
    return from_bin_le(result_bin);
}

mpz_class sub(const mpz_class& x, const mpz_class& y, size_t bin_size) {
    auto x_bin = to_bin_le(x, bin_size);
    auto y_bin = to_bin_le(y, bin_size);

    // Compute NOT(y) for subtraction via addition
    std::vector<bool> y_bin_not;
    y_bin_not.reserve(bin_size);
    for (size_t i = 0; i < bin_size; ++i) {
        y_bin_not.push_back(!y_bin[i]);
    }

    // x - y = x + NOT(y) + 1 (2's complement)
    auto result_bin = adder(x_bin, y_bin_not, true, bin_size);
    return from_bin_le(result_bin);
}

bool lt(const mpz_class& x, const mpz_class& y, size_t bin_size) {
    // x < y iff (x - y) has sign bit set
    mpz_class diff = sub(x, y, bin_size);
    return sign_bit(diff, bin_size);
}

mpz_class select(bool cond, const mpz_class& x, const mpz_class& y, size_t bin_size) {
    // result = y + cond * (x - y)
    // This is constant-time because we always compute both paths
    mpz_class diff = sub(x, y, bin_size);
    mpz_class cond_val = cond ? 1 : 0;
    mpz_class scaled = cond_val * diff;

    // Need to mask the scaled value to bin_size bits for proper 2's complement
    auto scaled_bin = to_bin_le(scaled, bin_size);
    mpz_class masked_scaled = from_bin_le(scaled_bin);

    return add(y, masked_scaled, bin_size);
}

bool verify_bin_size(const mpz_class& x, size_t bin_size) {
    // Check if x can be represented in bin_size bits (2's complement)
    // Valid range: [-2^(bin_size-1), 2^(bin_size-1) - 1]
    mpz_class shifted = x >> static_cast<mp_bitcnt_t>(bin_size);
    std::string shifted_str = shifted.get_str(2);
    return shifted_str == "0" || shifted_str == "-1";
}

size_t get_bin_size(const mpz_class& max_value) {
    // floor(log2(max)) + 1 for unsigned elements
    // +1 for signed elements
    // +1 to account for the diff of two negative elements
    if (max_value <= 0) {
        return 3;  // Minimum size
    }

    size_t bits = mpz_sizeinbase(max_value.get_mpz_t(), 2);
    return bits + 3;
}

mpz_class field_add(const mpz_class& x,
                    const mpz_class& y,
                    const mpz_class& p,
                    size_t bin_size) {
    mpz_class sum = add(x, y, bin_size);
    bool needs_reduction = !lt(sum, p, bin_size);
    return select(needs_reduction, sub(sum, p, bin_size), sum, bin_size);
}

mpz_class field_sub(const mpz_class& x,
                    const mpz_class& y,
                    const mpz_class& p,
                    size_t bin_size) {
    mpz_class diff = sub(x, y, bin_size);
    bool is_negative = sign_bit(diff, bin_size);
    return select(is_negative, add(diff, p, bin_size), diff, bin_size);
}

}  // namespace rescue::ct
