/**
 * @file bench_rescue.cpp
 * @brief Performance benchmarks for Rescue cipher operations.
 */

#include <rescue/rescue.hpp>

#include <benchmark/benchmark.h>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

using namespace rescue;
using json = nlohmann::json;

// Global JSON object to store results
json benchmark_results;

// ============================================================================
// Field Arithmetic Benchmarks
// ============================================================================

static void BM_FieldAddition(benchmark::State& state) {
    Fp a = Fp::random();
    Fp b = Fp::random();

    for (auto _ : state) {
        Fp result = a + b;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_FieldAddition);

static void BM_FieldMultiplication(benchmark::State& state) {
    Fp a = Fp::random();
    Fp b = Fp::random();

    for (auto _ : state) {
        Fp result = a * b;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_FieldMultiplication);

static void BM_FieldInversion(benchmark::State& state) {
    Fp a = Fp::random();
    while (a.is_zero()) {
        a = Fp::random();
    }

    for (auto _ : state) {
        Fp result = a.inv();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_FieldInversion);

static void BM_FieldExponentiation(benchmark::State& state) {
    Fp base = Fp::random();
    mpz_class exp("12345678901234567890");

    for (auto _ : state) {
        Fp result = base.pow(exp);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_FieldExponentiation);

// ============================================================================
// Matrix Benchmarks
// ============================================================================

static void BM_MatrixMultiplication_5x5(benchmark::State& state) {
    Matrix a = Matrix::random(5, 5);
    Matrix b = Matrix::random(5, 5);

    for (auto _ : state) {
        Matrix result = a.mat_mul(b);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_MatrixMultiplication_5x5);

static void BM_MatrixMultiplication_12x12(benchmark::State& state) {
    Matrix a = Matrix::random(12, 12);
    Matrix b = Matrix::random(12, 12);

    for (auto _ : state) {
        Matrix result = a.mat_mul(b);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_MatrixMultiplication_12x12);

static void BM_MatrixPow(benchmark::State& state) {
    Matrix a = Matrix::random(5, 5);
    mpz_class exp(5);  // Alpha = 5

    for (auto _ : state) {
        Matrix result = a.pow(exp);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_MatrixPow);

// ============================================================================
// Rescue Permutation Benchmarks
// ============================================================================

static void BM_RescuePermutation_Cipher(benchmark::State& state) {
    std::vector<Fp> key = {Fp(1), Fp(2), Fp(3), Fp(4), Fp(5)};
    RescueDesc desc(key);

    std::vector<Fp> input_data;
    for (size_t i = 0; i < 5; ++i) {
        input_data.push_back(Fp::random());
    }
    Matrix input(input_data);

    for (auto _ : state) {
        Matrix result = desc.permute(input);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_RescuePermutation_Cipher);

static void BM_RescuePermutation_Hash(benchmark::State& state) {
    RescueDesc desc(12, 5);

    std::vector<Fp> input_data(12, Fp::ZERO);
    for (size_t i = 0; i < 12; ++i) {
        input_data[i] = Fp::random();
    }
    Matrix input(input_data);

    for (auto _ : state) {
        Matrix result = desc.permute(input);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_RescuePermutation_Hash);

// ============================================================================
// Hash Benchmarks
// ============================================================================

static void BM_RescueHash_ShortMessage(benchmark::State& state) {
    RescuePrimeHash hasher;
    std::vector<Fp> msg = {Fp(1), Fp(2), Fp(3)};

    for (auto _ : state) {
        auto digest = hasher.digest(msg);
        benchmark::DoNotOptimize(digest);
    }
}
BENCHMARK(BM_RescueHash_ShortMessage);

static void BM_RescueHash_MediumMessage(benchmark::State& state) {
    RescuePrimeHash hasher;
    std::vector<Fp> msg;
    for (int i = 0; i < 20; ++i) {
        msg.push_back(Fp(static_cast<uint64_t>(i)));
    }

    for (auto _ : state) {
        auto digest = hasher.digest(msg);
        benchmark::DoNotOptimize(digest);
    }
}
BENCHMARK(BM_RescueHash_MediumMessage);

static void BM_RescueHash_LongMessage(benchmark::State& state) {
    RescuePrimeHash hasher;
    std::vector<Fp> msg;
    for (int i = 0; i < 100; ++i) {
        msg.push_back(Fp(static_cast<uint64_t>(i)));
    }

    for (auto _ : state) {
        auto digest = hasher.digest(msg);
        benchmark::DoNotOptimize(digest);
    }
}
BENCHMARK(BM_RescueHash_LongMessage);

// ============================================================================
// Cipher Benchmarks
// ============================================================================

static void BM_RescueCipher_Construction(benchmark::State& state) {
    auto secret = random_bytes<32>();

    for (auto _ : state) {
        RescueCipher cipher(secret);
        benchmark::DoNotOptimize(cipher);
    }
}
BENCHMARK(BM_RescueCipher_Construction);

static void BM_RescueCipher_Encrypt_1Block(benchmark::State& state) {
    auto secret = random_bytes<32>();
    RescueCipher cipher(secret);
    auto nonce = generate_nonce();

    std::vector<Fp> plaintext;
    for (size_t i = 0; i < RESCUE_CIPHER_BLOCK_SIZE; ++i) {
        plaintext.push_back(Fp::random());
    }

    for (auto _ : state) {
        auto ciphertext = cipher.encrypt_raw(plaintext, nonce);
        benchmark::DoNotOptimize(ciphertext);
    }
}
BENCHMARK(BM_RescueCipher_Encrypt_1Block);

static void BM_RescueCipher_Encrypt_10Blocks(benchmark::State& state) {
    auto secret = random_bytes<32>();
    RescueCipher cipher(secret);
    auto nonce = generate_nonce();

    std::vector<Fp> plaintext;
    for (size_t i = 0; i < 10 * RESCUE_CIPHER_BLOCK_SIZE; ++i) {
        plaintext.push_back(Fp::random());
    }

    for (auto _ : state) {
        auto ciphertext = cipher.encrypt_raw(plaintext, nonce);
        benchmark::DoNotOptimize(ciphertext);
    }
}
BENCHMARK(BM_RescueCipher_Encrypt_10Blocks);

static void BM_RescueCipher_Decrypt_1Block(benchmark::State& state) {
    auto secret = random_bytes<32>();
    RescueCipher cipher(secret);
    auto nonce = generate_nonce();

    std::vector<Fp> plaintext;
    for (size_t i = 0; i < RESCUE_CIPHER_BLOCK_SIZE; ++i) {
        plaintext.push_back(Fp::random());
    }
    auto ciphertext = cipher.encrypt_raw(plaintext, nonce);

    for (auto _ : state) {
        auto decrypted = cipher.decrypt_raw(ciphertext, nonce);
        benchmark::DoNotOptimize(decrypted);
    }
}
BENCHMARK(BM_RescueCipher_Decrypt_1Block);

// ============================================================================
// Constant-Time Operations Benchmarks
// ============================================================================

static void BM_ConstantTimeFieldAdd(benchmark::State& state) {
    Fp a = Fp::random();
    Fp b = Fp::random();
    size_t bin_size = ct::get_bin_size(Fp::P - 1);

    for (auto _ : state) {
        auto result = ct::field_add(a.value(), b.value(), Fp::P, bin_size);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ConstantTimeFieldAdd);

static void BM_ConstantTimeFieldSub(benchmark::State& state) {
    Fp a = Fp::random();
    Fp b = Fp::random();
    size_t bin_size = ct::get_bin_size(Fp::P - 1);

    for (auto _ : state) {
        auto result = ct::field_sub(a.value(), b.value(), Fp::P, bin_size);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ConstantTimeFieldSub);

static void BM_ConstantTimeLt(benchmark::State& state) {
    Fp a = Fp::random();
    Fp b = Fp::random();
    size_t bin_size = ct::get_bin_size(Fp::P - 1);

    for (auto _ : state) {
        bool result = ct::lt(a.value(), b.value(), bin_size);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ConstantTimeLt);

// ============================================================================
// Throughput Benchmarks
// ============================================================================

static void BM_RescueCipher_Throughput(benchmark::State& state) {
    auto secret = random_bytes<32>();
    RescueCipher cipher(secret);
    auto nonce = generate_nonce();

    // state.range(0) is number of field elements
    size_t n_elements = static_cast<size_t>(state.range(0));
    std::vector<Fp> plaintext;
    for (size_t i = 0; i < n_elements; ++i) {
        plaintext.push_back(Fp::random());
    }

    for (auto _ : state) {
        auto ciphertext = cipher.encrypt_raw(plaintext, nonce);
        benchmark::DoNotOptimize(ciphertext);
    }

    // Report throughput in field elements per second
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                           static_cast<int64_t>(n_elements));
    
    // Also report bytes per second (32 bytes per field element)
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                           static_cast<int64_t>(n_elements) * 32);
}
BENCHMARK(BM_RescueCipher_Throughput)->Range(1, 1024);

// Custom reporter to capture results
class JsonReporter : public benchmark::BenchmarkReporter {
public:
    json results;
    
    bool ReportContext(const Context& context) override {
        results["platform"] = "C++";
        results["timestamp"] = []() {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
            return ss.str();
        }();
        return true;
    }
    
    void ReportRuns(const std::vector<Run>& reports) override {
        for (const auto& run : reports) {
            std::string name = run.benchmark_name();
            // Remove "BM_" prefix for consistency with JS benchmarks
            if (name.substr(0, 3) == "BM_") {
                name = name.substr(3);
            }
            
            json bench;
            bench["iterations"] = run.iterations;
            bench["mean_ns"] = run.GetAdjustedRealTime();
            bench["mean_us"] = run.GetAdjustedRealTime() / 1000.0;
            bench["mean_ms"] = run.GetAdjustedRealTime() / 1000000.0;
            
            if (run.counters.count("items_per_second")) {
                bench["elements_per_second"] = run.counters.at("items_per_second").value;
            }
            if (run.counters.count("bytes_per_second")) {
                bench["megabytes_per_second"] = run.counters.at("bytes_per_second").value / (1024.0 * 1024.0);
            }
            
            results["benchmarks"][name] = bench;
        }
    }
    
    void Finalize() override {
        std::ofstream file("benchmark_results_cpp.json");
        file << results.dump(2);
        file.close();
        std::cout << "\nResults saved to benchmark_results_cpp.json\n";
    }
};

int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    
    // Run with console reporter first
    benchmark::RunSpecifiedBenchmarks();
    
    // Then run again with JSON reporter to capture results
    JsonReporter json_reporter;
    benchmark::RunSpecifiedBenchmarks(&json_reporter);
    json_reporter.Finalize();
    
    benchmark::Shutdown();
    return 0;
}
