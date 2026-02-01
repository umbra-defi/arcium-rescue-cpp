/**
 * Test SHAKE256 output to compare with JS
 */

#include <rescue/utils.hpp>
#include <rescue/field.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>

using namespace rescue;

std::string to_hex(const uint8_t* data, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; i++) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::string fp_to_hex(const Fp& val) {
    auto arr = val.to_bytes();
    std::stringstream ss;
    for (auto b : arr) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    return ss.str();
}

int main() {
    std::cout << "=== SHAKE256 Test (C++) ===\n\n";

    // Test with the same initialization as cipher mode
    Shake256 hasher;
    hasher.update("encrypt everything, compute anything");

    // We need to squeeze all at once - our implementation doesn't support incremental XOF
    // Extract 3 * 48 = 144 bytes
    auto output = hasher.finalize(144);

    std::cout << "First 3 chunks (48 bytes each):\n\n";
    std::cout << "chunk #1: " << to_hex(output.data(), 48) << "\n";
    std::cout << "chunk #2: " << to_hex(output.data() + 48, 48) << "\n";
    std::cout << "chunk #3: " << to_hex(output.data() + 96, 48) << "\n";

    // Now get field elements using wide_bytes_to_fp logic
    std::cout << "\nFirst 5 field elements from XOF (using wide reduction):\n\n";
    
    Shake256 hasher2;
    hasher2.update("encrypt everything, compute anything");
    auto output2 = hasher2.finalize(5 * 48);

    for (int i = 0; i < 5; i++) {
        std::span<const uint8_t> chunk(output2.data() + i * 48, 48);
        
        // Implement wide_bytes_to_fp logic inline
        std::array<uint64_t, 8> wide_limbs = {0};
        for (size_t j = 0; j < 48; ++j) {
            wide_limbs[j / 8] |= static_cast<uint64_t>(chunk[j]) << ((j % 8) * 8);
        }
        
        uint256 low{wide_limbs[0], wide_limbs[1], wide_limbs[2], wide_limbs[3]};
        uint256 high{wide_limbs[4], wide_limbs[5], 0, 0};
        
        Fp elem = Fp(low) + Fp(high) * Fp(uint64_t{38});
        std::cout << "  elem[" << i << "] = " << fp_to_hex(elem) << "\n";
    }

    std::cout << "\n=== Done ===\n";
    return 0;
}
