#pragma once

/**
 * @file rescue.hpp
 * @brief Main public header for the Rescue cipher library.
 *
 * This header includes all public API components of the Rescue cipher library.
 * Include this single header to get access to the full API.
 *
 * @code
 * #include <rescue/rescue.hpp>
 *
 * int main() {
 *     // Use RescuePrimeHash for hashing
 *     rescue::RescuePrimeHash hasher;
 *     auto digest = hasher.digest({rescue::Fp(1), rescue::Fp(2), rescue::Fp(3)});
 *
 *     // Use RescueCipher for encryption
 *     auto secret = rescue::random_bytes<32>();
 *     rescue::RescueCipher cipher(secret);
 *     auto nonce = rescue::generate_nonce();
 *     auto ciphertext = cipher.encrypt({rescue::Fp(42)}, nonce);
 *     auto plaintext = cipher.decrypt(ciphertext, nonce);
 *
 *     return 0;
 * }
 * @endcode
 */

// Core field arithmetic
#include <rescue/field.hpp>

// Matrix operations
#include <rescue/matrix.hpp>

// Utility functions
#include <rescue/utils.hpp>

// Rescue core (permutation, parameters)
#include <rescue/rescue_desc.hpp>

// Rescue-Prime hash function
#include <rescue/rescue_hash.hpp>

// Rescue cipher (CTR mode)
#include <rescue/rescue_cipher.hpp>

/**
 * @namespace rescue
 * @brief Namespace containing all Rescue cipher library components.
 *
 * Main components:
 * - rescue::Fp - Field element over Curve25519 base field
 * - rescue::Matrix - Matrix operations over Fp
 * - rescue::RescuePrimeHash - Sponge-based hash function
 * - rescue::RescueCipher - Block cipher in CTR mode
 */
