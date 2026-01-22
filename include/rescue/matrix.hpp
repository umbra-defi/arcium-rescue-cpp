#pragma once

/**
 * @file matrix.hpp
 * @brief Matrix operations over finite fields.
 *
 * This file provides the Matrix class for linear algebra operations
 * over the Fp field, used in the Rescue permutation.
 */

#include <rescue/field.hpp>

#include <cstddef>
#include <initializer_list>
#include <ostream>
#include <span>
#include <vector>

namespace rescue {

/**
 * @brief Matrix over the finite field Fp.
 *
 * This class provides matrix operations for the Rescue permutation,
 * including multiplication, addition, element-wise power, and determinant.
 * Data is stored in row-major order.
 */
class Matrix {
public:
    /**
     * @brief Construct an empty matrix.
     */
    Matrix() : rows_(0), cols_(0) {}

    /**
     * @brief Construct a matrix with given dimensions, initialized to zero.
     * @param rows Number of rows.
     * @param cols Number of columns.
     */
    Matrix(size_t rows, size_t cols);

    /**
     * @brief Construct a matrix from a 2D vector of field elements.
     * @param data 2D vector of Fp elements (row-major).
     * @throws std::invalid_argument if rows have different lengths.
     */
    explicit Matrix(const std::vector<std::vector<Fp>>& data);

    /**
     * @brief Construct a matrix from a 2D vector of integers.
     * @param data 2D vector of bigints (row-major).
     * @throws std::invalid_argument if rows have different lengths.
     */
    explicit Matrix(const std::vector<std::vector<mpz_class>>& data);

    /**
     * @brief Construct a column vector from a 1D vector.
     * @param data Vector of field elements.
     */
    explicit Matrix(const std::vector<Fp>& data);

    /**
     * @brief Construct a column vector from a 1D vector of bigints.
     * @param data Vector of bigints.
     */
    explicit Matrix(const std::vector<mpz_class>& data);

    // Default copy/move operations
    Matrix(const Matrix&) = default;
    Matrix(Matrix&&) noexcept = default;
    Matrix& operator=(const Matrix&) = default;
    Matrix& operator=(Matrix&&) noexcept = default;
    ~Matrix() = default;

    /**
     * @brief Create an identity matrix of given size.
     * @param size Dimension of the square matrix.
     * @return Identity matrix.
     */
    [[nodiscard]] static Matrix identity(size_t size);

    /**
     * @brief Create a zero matrix of given dimensions.
     * @param rows Number of rows.
     * @param cols Number of columns.
     * @return Zero matrix.
     */
    [[nodiscard]] static Matrix zeros(size_t rows, size_t cols);

    /**
     * @brief Create a random matrix of given dimensions.
     * @param rows Number of rows.
     * @param cols Number of columns.
     * @return Random matrix.
     */
    [[nodiscard]] static Matrix random(size_t rows, size_t cols);

    // Dimensions

    /**
     * @brief Get number of rows.
     */
    [[nodiscard]] size_t rows() const { return rows_; }

    /**
     * @brief Get number of columns.
     */
    [[nodiscard]] size_t cols() const { return cols_; }

    /**
     * @brief Check if matrix is square.
     */
    [[nodiscard]] bool is_square() const { return rows_ == cols_; }

    /**
     * @brief Check if matrix is empty.
     */
    [[nodiscard]] bool empty() const { return data_.empty(); }

    // Element access

    /**
     * @brief Access element at (row, col) - const version.
     */
    [[nodiscard]] const Fp& at(size_t row, size_t col) const;

    /**
     * @brief Access element at (row, col) - mutable version.
     */
    [[nodiscard]] Fp& at(size_t row, size_t col);

    /**
     * @brief Access element using operator() - const version.
     */
    [[nodiscard]] const Fp& operator()(size_t row, size_t col) const { return at(row, col); }

    /**
     * @brief Access element using operator() - mutable version.
     */
    [[nodiscard]] Fp& operator()(size_t row, size_t col) { return at(row, col); }

    /**
     * @brief Get row as a vector - const version.
     */
    [[nodiscard]] std::vector<Fp> row(size_t row_idx) const;

    /**
     * @brief Get column as a vector.
     */
    [[nodiscard]] std::vector<Fp> col(size_t col_idx) const;

    // Arithmetic operations

    /**
     * @brief Matrix multiplication.
     * @param rhs Right-hand side matrix.
     * @return this * rhs.
     * @throws std::invalid_argument if dimensions are incompatible.
     */
    [[nodiscard]] Matrix mat_mul(const Matrix& rhs) const;

    /**
     * @brief Element-wise addition.
     * @param rhs Right-hand side matrix.
     * @param constant_time Use constant-time operations.
     * @return this + rhs (element-wise).
     * @throws std::invalid_argument if dimensions don't match.
     */
    [[nodiscard]] Matrix add(const Matrix& rhs, bool constant_time = false) const;

    /**
     * @brief Element-wise subtraction.
     * @param rhs Right-hand side matrix.
     * @param constant_time Use constant-time operations.
     * @return this - rhs (element-wise).
     * @throws std::invalid_argument if dimensions don't match.
     */
    [[nodiscard]] Matrix sub(const Matrix& rhs, bool constant_time = false) const;

    /**
     * @brief Element-wise exponentiation.
     * @param exp The exponent.
     * @return Matrix with each element raised to exp.
     */
    [[nodiscard]] Matrix pow(const mpz_class& exp) const;

    /**
     * @brief Element-wise exponentiation (uint64_t version).
     * @param exp The exponent.
     * @return Matrix with each element raised to exp.
     */
    [[nodiscard]] Matrix pow(uint64_t exp) const;

    /**
     * @brief Scalar multiplication.
     * @param scalar The scalar to multiply by.
     * @return this * scalar (element-wise).
     */
    [[nodiscard]] Matrix scalar_mul(const Fp& scalar) const;

    /**
     * @brief Compute the determinant using Gaussian elimination.
     * @return The determinant of this matrix.
     * @throws std::invalid_argument if matrix is not square.
     *
     * @note This uses the algorithm from the TypeScript reference,
     *       which partitions rows by leading zeros.
     */
    [[nodiscard]] Fp det() const;

    /**
     * @brief Transpose the matrix.
     * @return The transposed matrix.
     */
    [[nodiscard]] Matrix transpose() const;

    // Operator overloads

    Matrix operator+(const Matrix& rhs) const { return add(rhs); }
    Matrix operator-(const Matrix& rhs) const { return sub(rhs); }
    Matrix operator*(const Matrix& rhs) const { return mat_mul(rhs); }
    Matrix operator*(const Fp& scalar) const { return scalar_mul(scalar); }

    bool operator==(const Matrix& rhs) const;

    // Conversion

    /**
     * @brief Convert column vector to std::vector<Fp>.
     * @return Vector of field elements.
     * @throws std::logic_error if matrix is not a column vector.
     */
    [[nodiscard]] std::vector<Fp> to_vector() const;

    /**
     * @brief Get raw data (row-major).
     */
    [[nodiscard]] const std::vector<Fp>& data() const { return data_; }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Matrix& mat);

private:
    size_t rows_;
    size_t cols_;
    std::vector<Fp> data_;  // Row-major storage

    /**
     * @brief Get linear index from (row, col).
     */
    [[nodiscard]] size_t index(size_t row, size_t col) const { return row * cols_ + col; }
};

/**
 * @brief Create a column vector matrix from field elements.
 * @param elements The field elements.
 * @return Column vector matrix.
 */
[[nodiscard]] Matrix to_column_vector(const std::vector<Fp>& elements);

/**
 * @brief Create a column vector matrix from bigints.
 * @param elements The bigint elements.
 * @return Column vector matrix.
 */
[[nodiscard]] Matrix to_column_vector(const std::vector<mpz_class>& elements);

}  // namespace rescue
