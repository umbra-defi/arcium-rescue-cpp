/**
 * 100k Benchmark and Interop Test for Rescue Cipher (C++)
 * 
 * Reads test vectors from JavaScript, verifies interoperability,
 * and benchmarks C++ encryption/decryption performance.
 */

#include <rescue/rescue.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace rescue;

// Configuration
const size_t BATCH_SIZE = 1000;   // Smaller batches for more frequent progress
const size_t LOG_INTERVAL = 100;  // Log every N tests within a batch

/**
 * Get current timestamp string
 */
std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

/**
 * Log with timestamp
 */
void log(const std::string& msg) {
    std::cout << "[" << timestamp() << "] " << msg << "\n";
}

/**
 * Convert hex string to byte vector (little-endian)
 */
std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.length() / 2);
    for (size_t i = 0; i < hex.length(); i += 2) {
        uint8_t byte = static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

/**
 * Convert byte vector to hex string
 */
std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    for (auto b : bytes) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    return ss.str();
}

/**
 * Convert hex string (little-endian) to Fp
 */
Fp hex_to_fp(const std::string& hex) {
    std::vector<uint8_t> bytes = hex_to_bytes(hex);
    return Fp::from_bytes(std::span<const uint8_t>(bytes));
}

/**
 * Convert Fp to hex string (little-endian)
 */
std::string fp_to_hex(const Fp& val) {
    auto arr = val.to_bytes();
    std::vector<uint8_t> bytes(arr.begin(), arr.end());
    return bytes_to_hex(bytes);
}

/**
 * Format number with commas
 */
std::string format_number(size_t n) {
    std::string s = std::to_string(n);
    int insert_pos = static_cast<int>(s.length()) - 3;
    while (insert_pos > 0) {
        s.insert(insert_pos, ",");
        insert_pos -= 3;
    }
    return s;
}

/**
 * Format bytes to human readable
 */
std::string format_bytes(size_t bytes) {
    std::stringstream ss;
    if (bytes < 1024) {
        ss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        ss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        ss << std::fixed << std::setprecision(1) << (bytes / 1024.0 / 1024.0) << " MB";
    } else {
        ss << std::fixed << std::setprecision(2) << (bytes / 1024.0 / 1024.0 / 1024.0) << " GB";
    }
    return ss.str();
}

int main(int argc, char* argv[]) {
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::nanoseconds;
    
    auto program_start = Clock::now();
    
    std::string input_file = "test_vectors_100k.ndjson";
    
    if (argc > 1) {
        input_file = argv[1];
    }

    std::cout << "\n";
    std::cout << std::string(80, '=') << "\n";
    std::cout << "  RESCUE CIPHER - C++ 100k Benchmark + Interop Test\n";
    std::cout << std::string(80, '=') << "\n";
    log("Starting benchmark");
    log("Configuration:");
    log("  - Input file: " + input_file);
    log("  - Batch size: " + format_number(BATCH_SIZE));
    log("  - Log interval: " + format_number(LOG_INTERVAL));
    std::cout << std::string(80, '-') << "\n";

    // Open file
    log("Opening input file...");
    std::ifstream ifs(input_file);
    if (!ifs.is_open()) {
        log("ERROR: Could not open " + input_file);
        return 1;
    }
    
    // Get file size
    ifs.seekg(0, std::ios::end);
    size_t file_size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    log("  File size: " + format_bytes(file_size));

    // Read metadata (first line)
    log("Reading metadata...");
    std::string line;
    std::getline(ifs, line);
    json metadata = json::parse(line);

    size_t num_tests = metadata["num_tests"];
    log("  Number of tests: " + format_number(num_tests));
    log("  Field: p = 2^255 - 19");
    
    // Print JS benchmark results if available
    if (metadata.contains("benchmark_results")) {
        auto& js_bench = metadata["benchmark_results"];
        log("  JS Platform: " + js_bench["platform"].get<std::string>());
        log("  JS Node version: " + js_bench["node_version"].get<std::string>());
    }
    std::cout << "\n";

    // Benchmark variables
    size_t total_elements = 0;
    size_t passed = 0;
    size_t failed = 0;
    size_t encrypt_mismatches = 0;
    size_t decrypt_mismatches = 0;

    Duration total_encrypt_time{0};
    Duration total_decrypt_time{0};
    Duration total_parse_time{0};
    Duration total_cipher_init_time{0};
    Duration total_verify_time{0};

    size_t num_batches = (num_tests + BATCH_SIZE - 1) / BATCH_SIZE;
    size_t batch_num = 0;
    size_t test_count = 0;
    size_t batch_elements = 0;
    
    Duration batch_encrypt_time{0};
    Duration batch_decrypt_time{0};
    Duration batch_parse_time{0};
    Duration batch_cipher_init_time{0};
    Duration batch_verify_time{0};
    
    auto batch_start = Clock::now();

    log(std::string(79, '='));
    log("PHASE 1: Loading Test Vectors & Benchmarking");
    log(std::string(79, '='));
    std::cout << "\n";
    
    log("Batch " + std::to_string(batch_num + 1) + "/" + std::to_string(num_batches) + 
        " starting (tests 0-" + std::to_string(std::min(BATCH_SIZE, num_tests) - 1) + ")");

    // Process test vectors
    while (std::getline(ifs, line)) {
        // Parse JSON
        auto parse_start = Clock::now();
        json test_vec = json::parse(line);
        
        int test_id = test_vec["id"];
        
        // Parse inputs
        std::vector<uint8_t> shared_secret = hex_to_bytes(test_vec["shared_secret"]);
        std::vector<uint8_t> nonce = hex_to_bytes(test_vec["nonce"]);
        
        // Parse plaintext
        std::vector<Fp> plaintext;
        plaintext.reserve(test_vec["plaintext"].size());
        for (const auto& pt_hex : test_vec["plaintext"]) {
            plaintext.push_back(hex_to_fp(pt_hex.get<std::string>()));
        }
        
        // Parse expected ciphertext from JS
        std::vector<Fp> expected_ciphertext;
        expected_ciphertext.reserve(test_vec["ciphertext"].size());
        for (const auto& ct_hex : test_vec["ciphertext"]) {
            expected_ciphertext.push_back(hex_to_fp(ct_hex.get<std::string>()));
        }
        auto parse_end = Clock::now();
        batch_parse_time += std::chrono::duration_cast<Duration>(parse_end - parse_start);

        total_elements += plaintext.size();
        batch_elements += plaintext.size();

        // Create cipher
        auto cipher_init_start = Clock::now();
        RescueCipher cipher(shared_secret);
        auto cipher_init_end = Clock::now();
        batch_cipher_init_time += std::chrono::duration_cast<Duration>(cipher_init_end - cipher_init_start);

        // Benchmark encryption
        auto enc_start = Clock::now();
        std::vector<Fp> cpp_ciphertext = cipher.encrypt_raw(plaintext, nonce);
        auto enc_end = Clock::now();
        batch_encrypt_time += std::chrono::duration_cast<Duration>(enc_end - enc_start);

        // Benchmark decryption
        auto dec_start = Clock::now();
        std::vector<Fp> cpp_decrypted = cipher.decrypt_raw(expected_ciphertext, nonce);
        auto dec_end = Clock::now();
        batch_decrypt_time += std::chrono::duration_cast<Duration>(dec_end - dec_start);

        // Verify encryption matches JS
        auto verify_start = Clock::now();
        bool enc_match = (cpp_ciphertext.size() == expected_ciphertext.size());
        if (enc_match) {
            for (size_t i = 0; i < cpp_ciphertext.size(); i++) {
                if (cpp_ciphertext[i] != expected_ciphertext[i]) {
                    enc_match = false;
                    break;
                }
            }
        }

        // Verify decryption matches original plaintext
        bool dec_match = (cpp_decrypted.size() == plaintext.size());
        if (dec_match) {
            for (size_t i = 0; i < cpp_decrypted.size(); i++) {
                if (cpp_decrypted[i] != plaintext[i]) {
                    dec_match = false;
                    break;
                }
            }
        }
        auto verify_end = Clock::now();
        batch_verify_time += std::chrono::duration_cast<Duration>(verify_end - verify_start);

        if (enc_match && dec_match) {
            passed++;
        } else {
            failed++;
            if (!enc_match) encrypt_mismatches++;
            if (!dec_match) decrypt_mismatches++;
            
            // Print first few failures
            if (failed <= 5) {
                log("  ERROR: FAILED test " + std::to_string(test_id) + ": " +
                    "encrypt=" + (enc_match ? "OK" : "MISMATCH") +
                    ", decrypt=" + (dec_match ? "OK" : "MISMATCH"));
            }
        }

        test_count++;
        size_t within_batch = test_count - (batch_num * BATCH_SIZE);
        
        // Progress within batch
        if (within_batch % LOG_INTERVAL == 0) {
            double pct_batch = (static_cast<double>(within_batch) / BATCH_SIZE) * 100.0;
            double pct_total = (static_cast<double>(test_count) / num_tests) * 100.0;
            std::cout << "\r  [" << timestamp() << "]   Progress: " 
                      << within_batch << "/" << BATCH_SIZE 
                      << " (" << std::fixed << std::setprecision(0) << pct_batch << "%) | Total: "
                      << format_number(test_count) << "/" << format_number(num_tests) 
                      << " (" << std::setprecision(1) << pct_total << "%)" << std::flush;
        }
        
        // Batch complete
        if (test_count % BATCH_SIZE == 0 || test_count == num_tests) {
            std::cout << "\n";  // New line after progress
            
            auto batch_end = Clock::now();
            auto batch_total = std::chrono::duration_cast<std::chrono::milliseconds>(batch_end - batch_start);
            
            total_encrypt_time += batch_encrypt_time;
            total_decrypt_time += batch_decrypt_time;
            total_parse_time += batch_parse_time;
            total_cipher_init_time += batch_cipher_init_time;
            total_verify_time += batch_verify_time;
            
            log("Batch " + std::to_string(batch_num + 1) + "/" + std::to_string(num_batches) + " complete:");
            log("  - Time: " + std::to_string(batch_total.count()) + "ms");
            log("  - Elements: " + format_number(batch_elements));
            log("  - JSON parsing: " + std::to_string(batch_parse_time.count() / 1000000) + "ms");
            log("  - Cipher init: " + std::to_string(batch_cipher_init_time.count() / 1000000) + "ms");
            log("  - Encryption: " + std::to_string(batch_encrypt_time.count() / 1000000) + "ms");
            log("  - Decryption: " + std::to_string(batch_decrypt_time.count() / 1000000) + "ms");
            log("  - Verification: " + std::to_string(batch_verify_time.count() / 1000000) + "ms");
            log("  - Passed so far: " + format_number(passed) + " | Failed: " + format_number(failed));
            
            double progress = 100.0 * test_count / num_tests;
            log("  - Overall progress: " + std::to_string(static_cast<int>(progress)) + "%");
            std::cout << "\n";
            
            // Reset batch counters
            batch_num++;
            batch_elements = 0;
            batch_encrypt_time = Duration{0};
            batch_decrypt_time = Duration{0};
            batch_parse_time = Duration{0};
            batch_cipher_init_time = Duration{0};
            batch_verify_time = Duration{0};
            batch_start = Clock::now();
            
            // Start next batch logging
            if (test_count < num_tests) {
                size_t next_end = std::min(test_count + BATCH_SIZE, num_tests);
                log("Batch " + std::to_string(batch_num + 1) + "/" + std::to_string(num_batches) + 
                    " starting (tests " + format_number(test_count) + "-" + format_number(next_end - 1) + ")");
            }
        }
    }

    ifs.close();

    // Calculate statistics
    double total_enc_sec = total_encrypt_time.count() / 1e9;
    double total_dec_sec = total_decrypt_time.count() / 1e9;
    double avg_enc_us = total_encrypt_time.count() / static_cast<double>(num_tests) / 1000.0;
    double avg_dec_us = total_decrypt_time.count() / static_cast<double>(num_tests) / 1000.0;
    double enc_throughput = total_elements / total_enc_sec;
    double dec_throughput = total_elements / total_dec_sec;

    // Print results
    log(std::string(79, '='));
    log("PHASE 1 COMPLETE: Benchmark Results");
    log(std::string(79, '='));
    std::cout << "\n";
    
    log("Summary Statistics:");
    log("  Total test cases:        " + format_number(num_tests));
    log("  Total elements:          " + format_number(total_elements));
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << static_cast<double>(total_elements) / num_tests;
    log("  Avg elements/test:       " + ss.str());
    std::cout << "\n";
    
    log("Timing Breakdown:");
    ss.str(""); ss << std::fixed << std::setprecision(3) << (total_parse_time.count() / 1e9);
    log("  JSON parsing:            " + ss.str() + " s");
    ss.str(""); ss << std::fixed << std::setprecision(3) << (total_cipher_init_time.count() / 1e9);
    log("  Cipher initialization:   " + ss.str() + " s");
    ss.str(""); ss << std::fixed << std::setprecision(3) << total_enc_sec;
    log("  Total encrypt time:      " + ss.str() + " s");
    ss.str(""); ss << std::fixed << std::setprecision(3) << total_dec_sec;
    log("  Total decrypt time:      " + ss.str() + " s");
    ss.str(""); ss << std::fixed << std::setprecision(3) << (total_verify_time.count() / 1e9);
    log("  Verification time:       " + ss.str() + " s");
    std::cout << "\n";
    
    log("Per-Operation Averages:");
    ss.str(""); ss << std::fixed << std::setprecision(3) << avg_enc_us;
    log("  Avg encrypt time/test:   " + ss.str() + " μs");
    ss.str(""); ss << std::fixed << std::setprecision(3) << avg_dec_us;
    log("  Avg decrypt time/test:   " + ss.str() + " μs");
    std::cout << "\n";
    
    log("Throughput:");
    ss.str(""); ss << std::fixed << std::setprecision(0) << enc_throughput;
    log("  Encrypt throughput:      " + ss.str() + " elements/s");
    ss.str(""); ss << std::fixed << std::setprecision(0) << dec_throughput;
    log("  Decrypt throughput:      " + ss.str() + " elements/s");
    std::cout << "\n";

    // Interop results
    log(std::string(79, '='));
    log("Interoperability Results");
    log(std::string(79, '='));
    log("  Passed:                  " + format_number(passed));
    log("  Failed:                  " + format_number(failed));
    if (failed > 0) {
        log("    - Encryption mismatches: " + std::to_string(encrypt_mismatches));
        log("    - Decryption mismatches: " + std::to_string(decrypt_mismatches));
    }
    ss.str(""); ss << std::fixed << std::setprecision(2) << (100.0 * passed / num_tests);
    log("  Success rate:            " + ss.str() + "%");
    std::cout << "\n";

    // Write results to JSON
    log(std::string(79, '='));
    log("PHASE 2: Writing Results");
    log(std::string(79, '='));
    
    json results;
    results["description"] = "100k Rescue Cipher Benchmark Results (C++)";
    results["platform"] = "C++";
    results["benchmark_results"] = {
        {"total_tests", num_tests},
        {"total_elements", total_elements},
        {"total_encrypt_time_ns", total_encrypt_time.count()},
        {"total_decrypt_time_ns", total_decrypt_time.count()},
        {"total_parse_time_ns", total_parse_time.count()},
        {"total_cipher_init_time_ns", total_cipher_init_time.count()},
        {"total_verify_time_ns", total_verify_time.count()},
        {"avg_encrypt_time_us", avg_enc_us},
        {"avg_decrypt_time_us", avg_dec_us},
        {"encrypt_throughput_elements_per_sec", enc_throughput},
        {"decrypt_throughput_elements_per_sec", dec_throughput}
    };
    results["interop_results"] = {
        {"passed", passed},
        {"failed", failed},
        {"encrypt_mismatches", encrypt_mismatches},
        {"decrypt_mismatches", decrypt_mismatches},
        {"success_rate_percent", 100.0 * passed / num_tests}
    };
    results["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

    std::string output_file = "benchmark_results_100k_cpp.json";
    log("Writing results to " + output_file + "...");
    std::ofstream ofs(output_file);
    ofs << std::setw(2) << results << std::endl;
    ofs.close();
    log("  Results written successfully");
    std::cout << "\n";

    auto program_end = Clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(program_end - program_start);
    
    log(std::string(79, '='));
    log("BENCHMARK COMPLETE");
    log(std::string(79, '='));
    ss.str(""); ss << std::fixed << std::setprecision(2) << (total_time.count() / 1000.0);
    log("Total execution time: " + ss.str() + " seconds");
    std::cout << "\n";

    return failed > 0 ? 1 : 0;
}
