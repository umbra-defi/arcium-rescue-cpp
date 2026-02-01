# Architecture

This document describes the internal architecture of the Rescue Cipher C++ implementation.

## Overview

The library is organized into three layers:

1. **Public API Layer** - User-facing classes for encryption, decryption, and hashing
2. **Core Components** - Internal implementation of the Rescue permutation and field operations
3. **Low-Level Primitives** - Constant-time arithmetic, big integers, and cryptographic utilities

## Component Diagram

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
│  ConstantTime Ops      │  uint256     │  SHAKE256           │
│  - ct::add()           │  (BigInt)    │  (OpenSSL)          │
│  - ct::sub()           │              │                     │
│  - ct::select()        │              │                     │
└─────────────────────────────────────────────────────────────┘
```

## File Organization

### Public Headers (`include/rescue/`)

| File | Description |
|------|-------------|
| `rescue.hpp` | Main include file - includes all public API |
| `field.hpp` | `Fp` class for field element arithmetic |
| `matrix.hpp` | Matrix operations over the field |
| `rescue_hash.hpp` | `RescuePrimeHash` sponge-based hash function |
| `rescue_cipher.hpp` | `RescueCipher` block cipher in CTR mode |
| `rescue_desc.hpp` | `RescueDesc` permutation implementation |
| `utils.hpp` | Utility functions (SHAKE256, serialization, RNG) |

### Internal Headers (`include/rescue/detail/`)

| File | Description |
|------|-------------|
| `uint256.hpp` | 256-bit unsigned integer implementation |
| `fp_impl.hpp` | Optimized field arithmetic for p = 2^255 - 19 |
| `mds_precomputed.hpp` | Precomputed MDS matrices |

### Source Files (`src/`)

Implementation files corresponding to each public header.

## Key Design Decisions

### Constant-Time Operations

All cryptographic operations are implemented using constant-time primitives:

- No secret-dependent branches
- No secret-dependent memory accesses
- Timing-safe comparisons and selections

### Field Arithmetic Optimizations

The field p = 2^255 - 19 allows for efficient reduction:
- 2^255 ≡ 19 (mod p), so 2^256 ≡ 38 (mod p)
- Reduction requires only multiplication by 38 and conditional subtraction

### Precomputed Constants

MDS matrices are precomputed at compile time to avoid runtime modular inversions during initialization.

## Data Flow

### Encryption Flow

```
Shared Secret (32 bytes)
         │
         ▼
┌─────────────────┐
│  Key Derivation │  (RescuePrimeHash)
└────────┬────────┘
         │
         ▼
    Cipher Key
         │
         ├──────────────────┐
         ▼                  ▼
┌─────────────────┐  ┌─────────────┐
│   Counter Block │  │  Plaintext  │
│   (nonce + ctr) │  │             │
└────────┬────────┘  └──────┬──────┘
         │                  │
         ▼                  │
┌─────────────────┐         │
│ Rescue Permute  │         │
└────────┬────────┘         │
         │                  │
         ▼                  ▼
┌─────────────────────────────────┐
│            XOR                  │
└────────────────┬────────────────┘
                 │
                 ▼
            Ciphertext
```

### Hashing Flow

```
Message (array of Fp)
         │
         ▼
┌─────────────────┐
│     Padding     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   Absorb Phase  │  (rate = 7 elements per round)
│  ┌───────────┐  │
│  │  Permute  │←─┼── repeated until all absorbed
│  └───────────┘  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Squeeze Phase  │  (extract 5 elements)
└────────┬────────┘
         │
         ▼
   Digest (5 Fp elements)
```

## Security Properties

| Property | Level |
|----------|-------|
| Collision resistance (hash) | 256 bits |
| Preimage resistance (hash) | 256 bits |
| Encryption security (cipher) | 128 bits |
| Side-channel resistance | Constant-time |

## Dependencies

- **OpenSSL 3.0+**: SHAKE256 and SHA-256 implementations
- **C++23**: Modern language features (concepts, ranges, etc.)
