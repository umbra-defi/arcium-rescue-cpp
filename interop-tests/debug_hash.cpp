/**
 * Debug hash function parameters
 */

#include <rescue/rescue.hpp>
#include <rescue/rescue_desc.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>

using namespace rescue;

std::string fp_to_hex(const Fp& val) {
    auto arr = val.to_bytes();
    std::stringstream ss;
    for (auto b : arr) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    return ss.str();
}

std::string uint256_to_hex_le(const uint256& val) {
    // Output in little-endian hex (same as JS)
    std::stringstream ss;
    for (int i = 0; i < 4; i++) {
        uint64_t limb = val.limb(i);
        for (int j = 0; j < 8; j++) {
            ss << std::hex << std::setfill('0') << std::setw(2) 
               << static_cast<int>((limb >> (j * 8)) & 0xFF);
        }
    }
    return ss.str();
}

int main() {
    std::cout << "=== Hash Function Parameters ===\n\n";

    // Check the RescuePrimeHash parameters
    RescuePrimeHash hasher;
    
    std::cout << "Hash mode parameters:\n";
    std::cout << "  RESCUE_HASH_RATE = " << RESCUE_HASH_RATE << "\n";
    std::cout << "  RESCUE_HASH_CAPACITY = " << RESCUE_HASH_CAPACITY << "\n";
    std::cout << "  RESCUE_HASH_STATE_SIZE = " << RESCUE_HASH_STATE_SIZE << "\n";
    std::cout << "  RESCUE_HASH_DIGEST_LENGTH = " << RESCUE_HASH_DIGEST_LENGTH << "\n";

    // Check alpha and alpha_inverse
    std::cout << "\n=== Alpha calculation ===\n";
    auto [alpha, alpha_inverse] = get_alpha_and_inverse(Fp::P);
    std::cout << "  alpha = " << uint256_to_hex_le(alpha) << " (decimal: " << alpha.limb(0) << ")\n";
    std::cout << "  alpha_inverse = " << uint256_to_hex_le(alpha_inverse) << "\n";
    
    // JS shows: c1cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc4c
    std::cout << "\n  JS alpha_inverse = c1cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc4c\n";
    
    // Verify: alpha * alpha_inverse mod (p-1) should be 1
    uint256 p_minus_1 = Fp::P - uint256::one();
    std::cout << "  p - 1 = " << uint256_to_hex_le(p_minus_1) << "\n";

    // Test hash with simple input
    std::cout << "\n=== Hash Test ===\n";
    std::vector<Fp> input = {Fp(uint64_t{1}), Fp(uint64_t{2}), Fp(uint64_t{3})};
    std::cout << "Input: [1, 2, 3]\n";
    
    auto output = hasher.digest(input);
    std::cout << "Output (" << output.size() << " elements):\n";
    for (size_t i = 0; i < output.size(); i++) {
        std::cout << "  [" << i << "] = " << fp_to_hex(output[i]) << "\n";
    }

    // Now check cipher mode parameters
    std::cout << "\n=== Cipher Mode Parameters ===\n";
    std::cout << "  RESCUE_CIPHER_BLOCK_SIZE = " << RESCUE_CIPHER_BLOCK_SIZE << "\n";

    // Create a cipher key to test
    std::vector<Fp> test_key = {
        Fp(uint64_t{1}), Fp(uint64_t{2}), Fp(uint64_t{3}), Fp(uint64_t{4}), Fp(uint64_t{5})
    };
    RescueDesc desc(test_key);
    
    std::cout << "\nCipher Desc with test key [1,2,3,4,5]:\n";
    std::cout << "  m = " << desc.m() << "\n";
    std::cout << "  nRounds = " << desc.n_rounds() << "\n";
    std::cout << "  alpha = " << uint256_to_hex_le(desc.alpha()) << "\n";
    std::cout << "  alpha_inverse = " << uint256_to_hex_le(desc.alpha_inverse()) << "\n";

    // Print MDS matrix first row
    std::cout << "\nMDS Matrix (first row):\n";
    const auto& mds = desc.mds_matrix();
    for (size_t j = 0; j < desc.m(); j++) {
        std::cout << "  mds[0][" << j << "] = " << fp_to_hex(mds(0, j)) << "\n";
    }

    // Print round keys
    const auto& rks = desc.round_keys();
    std::cout << "\nRound constants (first round key, " << rks.size() << " total):\n";
    if (!rks.empty()) {
        auto rk0 = rks[0].to_vector();
        for (size_t i = 0; i < rk0.size(); i++) {
            std::cout << "  rk0[" << i << "] = " << fp_to_hex(rk0[i]) << "\n";
        }
    }

    return 0;
}
