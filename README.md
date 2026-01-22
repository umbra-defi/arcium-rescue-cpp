# Rescue Cipher C++

A production-quality C++ implementation of the **Rescue cipher** over the Curve25519 base field, featuring full constant-time operations for side-channel resistance.

## Overview

Rescue is a SNARK-friendly symmetric cryptographic primitive designed for efficient evaluation in zero-knowledge proof systems. This implementation provides:

- **Rescue-Prime Hash**: Sponge-based hash function with 256-bit security
- **Rescue Cipher**: Block cipher in CTR mode with 128-bit security
- **Constant-Time Operations**: Full side-channel resistance using binary arithmetic

### Mathematical Foundation

| Parameter | Value |
|-----------|-------|
| Field | Curve25519 base field: p = 2²⁵⁵ - 19 |
| Alpha (α) | 5 (smallest prime not dividing p-1) |
| Block Size (cipher) | 5 field elements |
| State Size (hash) | 12 (rate=7, capacity=5) |
| Security Level | 128-bit (cipher), 256-bit (hash) |

## Requirements

- **C++23** compatible compiler (GCC 13+, Clang 16+, MSVC 2022+)
- **CMake** 3.25+
- **GMP** (GNU Multiple Precision Arithmetic Library)
- **OpenSSL** 3.0+ (for SHAKE256)
- **GoogleTest** (optional, for tests)
- **Google Benchmark** (optional, for benchmarks)

## Building

### Quick Start

```bash
# Clone the repository
git clone https://github.com/arcium/rescue-cipher-cpp.git
cd rescue-cipher-cpp

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .

# Run tests
ctest --output-on-failure
```

### Build Options

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DRESCUE_BUILD_TESTS=ON \
    -DRESCUE_BUILD_BENCHMARKS=ON \
    -DRESCUE_BUILD_EXAMPLES=ON
```

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install libgmp-dev libssl-dev
```

**macOS (Homebrew):**
```bash
brew install gmp openssl@3
```

**Arch Linux:**
```bash
sudo pacman -S gmp openssl
```

## Usage

### Basic Encryption/Decryption

```cpp
#include <rescue/rescue.hpp>

int main() {
    // Create shared secret (32 bytes)
    std::array<uint8_t, 32> shared_secret = /* derive from key exchange */;
    
    // Initialize cipher
    rescue::RescueCipher cipher(shared_secret);
    
    // Prepare plaintext (array of field elements)
    std::vector<rescue::Fp> plaintext = {
        rescue::Fp(42),
        rescue::Fp(1337),
        rescue::Fp(0xDEADBEEF)
    };
    
    // Generate nonce (16 bytes)
    std::array<uint8_t, 16> nonce = rescue::random_bytes<16>();
    
    // Encrypt
    auto ciphertext = cipher.encrypt(plaintext, nonce);
    
    // Decrypt
    auto decrypted = cipher.decrypt(ciphertext, nonce);
    
    // decrypted == plaintext
    return 0;
}
```

### Hashing

```cpp
#include <rescue/rescue.hpp>

int main() {
    rescue::RescuePrimeHash hasher;
    
    // Hash a message (array of field elements)
    std::vector<rescue::Fp> message = {
        rescue::Fp(1),
        rescue::Fp(2),
        rescue::Fp(3)
    };
    
    auto digest = hasher.digest(message);
    // digest is a vector of 5 field elements
    
    return 0;
}
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Public API Layer                        │
├─────────────────────────────────────────────────────────────┤
│  RescueCipher          │           RescuePrimeHash          │
│  - encrypt()           │           - digest()               │
│  - decrypt()           │                                    │
└────────────┬───────────┴───────────────┬────────────────────┘
             │                           │
             ▼                           ▼
┌─────────────────────────────────────────────────────────────┐
│                     Core Components                         │
├─────────────────────────────────────────────────────────────┤
│  RescueDesc            │  Matrix      │  Fp (Field)         │
│  - permute()           │  - matMul()  │  - add(), sub()     │
│  - permuteInverse()    │  - add()     │  - mul(), pow()     │
│  - MDS matrix          │  - pow()     │  - inv()            │
│  - round constants     │  - det()     │                     │
└────────────┬───────────┴──────┬───────┴─────────────────────┘
             │                  │
             ▼                  ▼
┌─────────────────────────────────────────────────────────────┐
│                   Low-Level Primitives                      │
├─────────────────────────────────────────────────────────────┤
│  ConstantTime Ops      │  GMP BigInt  │  SHAKE256           │
│  - ct::add()           │  mpz_class   │  (OpenSSL)          │
│  - ct::sub()           │              │                     │
│  - ct::select()        │              │                     │
└─────────────────────────────────────────────────────────────┘
```

## Security Considerations

### Constant-Time Implementation

All cryptographic operations are implemented using constant-time primitives to prevent timing side-channel attacks:

- Field arithmetic uses binary representation for comparisons and selections
- No secret-dependent branches or memory accesses
- Modular reduction uses constant-time selection

### Thread Safety

- `Fp` objects are immutable and thread-safe
- `RescueCipher` and `RescuePrimeHash` instances maintain internal state
- Create separate instances per thread for concurrent use

## References

1. [Rescue-Prime: a Standard Specification](https://eprint.iacr.org/2020/1143.pdf)
2. [Algebraic Hash Functions for SNARK Applications](https://tosc.iacr.org/index.php/ToSC/article/view/8695/8287)
3. [Curve25519 Specification](https://cr.yp.to/ecdh/curve25519-20060209.pdf)

## License

MIT License - see [LICENSE](LICENSE) for details.
