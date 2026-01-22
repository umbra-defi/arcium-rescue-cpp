#include <rescue/rescue_hash.hpp>

#include <rescue/matrix.hpp>

#include <stdexcept>

namespace rescue {

RescuePrimeHash::RescuePrimeHash()
    : rate_(RESCUE_HASH_RATE),
      capacity_(RESCUE_HASH_CAPACITY),
      digest_length_(RESCUE_HASH_DIGEST_LENGTH),
      desc_(RESCUE_HASH_STATE_SIZE, RESCUE_HASH_CAPACITY) {}

RescuePrimeHash::RescuePrimeHash(size_t rate, size_t capacity, size_t digest_length)
    : rate_(rate),
      capacity_(capacity),
      digest_length_(digest_length),
      desc_(rate + capacity, capacity) {
    if (rate == 0) {
        throw std::invalid_argument("Rate must be positive");
    }
    if (capacity == 0) {
        throw std::invalid_argument("Capacity must be positive");
    }
    if (digest_length == 0) {
        throw std::invalid_argument("Digest length must be positive");
    }
    if (digest_length > rate + capacity) {
        throw std::invalid_argument("Digest length cannot exceed state size");
    }
}

std::vector<Fp> RescuePrimeHash::digest(const std::vector<Fp>& message) const {
    // Apply padding: append 1, then zeros until length is multiple of rate
    std::vector<Fp> padded_message = message;
    padded_message.push_back(Fp::ONE);

    while (padded_message.size() % rate_ != 0) {
        padded_message.push_back(Fp::ZERO);
    }

    // Initialize state to all zeros
    size_t m = desc_.m();
    std::vector<Fp> state_vec(m, Fp::ZERO);
    Matrix state(state_vec);

    // Absorb phase: process message in rate-sized chunks
    size_t n_blocks = padded_message.size() / rate_;
    for (size_t block = 0; block < n_blocks; ++block) {
        // Create absorption vector (rate elements + capacity zeros)
        std::vector<Fp> absorb_vec;
        absorb_vec.reserve(m);

        for (size_t i = 0; i < rate_; ++i) {
            absorb_vec.push_back(padded_message[block * rate_ + i]);
        }
        for (size_t i = rate_; i < m; ++i) {
            absorb_vec.push_back(Fp::ZERO);
        }

        Matrix absorb(absorb_vec);

        // Add to state and permute (using constant-time addition)
        state = desc_.permute(state.add(absorb, true));
    }

    // Extract digest from state
    auto state_data = state.to_vector();
    std::vector<Fp> result;
    result.reserve(digest_length_);
    for (size_t i = 0; i < digest_length_; ++i) {
        result.push_back(state_data[i]);
    }

    return result;
}

std::vector<Fp> RescuePrimeHash::digest(const std::vector<mpz_class>& message) const {
    std::vector<Fp> fp_message;
    fp_message.reserve(message.size());
    for (const auto& val : message) {
        fp_message.emplace_back(val);
    }
    return digest(fp_message);
}

}  // namespace rescue
