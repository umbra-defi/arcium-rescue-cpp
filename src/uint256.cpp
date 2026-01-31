#include <rescue/uint256.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace rescue {

uint256::uint256(std::string_view hex) : limbs_{0, 0, 0, 0} {
    std::string str(hex);

    // Remove 0x prefix if present
    if (str.size() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str = str.substr(2);
    }

    // Remove leading zeros
    size_t start = str.find_first_not_of('0');
    if (start == std::string::npos) {
        return;  // All zeros or empty
    }
    str = str.substr(start);

    if (str.size() > 64) {
        throw std::overflow_error("Hex string too large for uint256");
    }

    // Pad to 64 characters (32 bytes)
    while (str.size() < 64) {
        str = "0" + str;
    }

    // Convert from big-endian hex to little-endian limbs
    auto hex_to_nibble = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
        throw std::invalid_argument("Invalid hex character");
    };

    // Process 16 hex chars (64 bits) per limb, from the end (little-endian)
    for (size_t i = 0; i < LIMBS; ++i) {
        size_t str_pos = 64 - (i + 1) * 16;
        uint64_t val = 0;
        for (size_t j = 0; j < 16; ++j) {
            val = (val << 4) | hex_to_nibble(str[str_pos + j]);
        }
        limbs_[i] = val;
    }
}

std::string uint256::to_hex() const {
    std::ostringstream oss;
    oss << "0x";

    bool leading = true;
    for (int i = LIMBS - 1; i >= 0; --i) {
        if (leading && limbs_[i] == 0 && i > 0) {
            continue;
        }
        if (leading) {
            oss << std::hex << limbs_[i];
            leading = false;
        } else {
            oss << std::setfill('0') << std::setw(16) << std::hex << limbs_[i];
        }
    }

    if (leading) {
        oss << "0";
    }

    return oss.str();
}

std::string uint256::to_string() const {
    if (is_zero()) {
        return "0";
    }

    // Division-based decimal conversion
    // We work with a copy and repeatedly divide by 10
    std::array<uint64_t, LIMBS> temp = limbs_;
    std::string result;

    auto is_temp_zero = [&temp]() {
        return (temp[0] | temp[1] | temp[2] | temp[3]) == 0;
    };

    while (!is_temp_zero()) {
        // Divide by 10, keeping track of remainder
        uint64_t remainder = 0;
        for (int i = LIMBS - 1; i >= 0; --i) {
#if defined(__SIZEOF_INT128__)
            unsigned __int128 dividend = (static_cast<unsigned __int128>(remainder) << 64) | temp[i];
            temp[i] = static_cast<uint64_t>(dividend / 10);
            remainder = static_cast<uint64_t>(dividend % 10);
#else
            // Portable version using 32-bit operations
            uint64_t high = (remainder << 32) | (temp[i] >> 32);
            uint64_t q_high = high / 10;
            uint64_t r_high = high % 10;

            uint64_t low = (r_high << 32) | (temp[i] & 0xFFFFFFFF);
            uint64_t q_low = low / 10;
            remainder = low % 10;

            temp[i] = (q_high << 32) | q_low;
#endif
        }
        result.push_back(static_cast<char>('0' + remainder));
    }

    std::reverse(result.begin(), result.end());
    return result;
}

std::ostream& operator<<(std::ostream& os, const uint256& val) {
    return os << val.to_string();
}

}  // namespace rescue
