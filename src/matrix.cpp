#include <rescue/matrix.hpp>

#include <rescue/constant_time.hpp>

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace rescue {

Matrix::Matrix(size_t rows, size_t cols)
    : rows_(rows), cols_(cols), data_(rows * cols, Fp::ZERO) {}

Matrix::Matrix(const std::vector<std::vector<Fp>>& data) {
    if (data.empty()) {
        rows_ = 0;
        cols_ = 0;
        return;
    }

    rows_ = data.size();
    cols_ = data[0].size();

    // Verify all rows have same length
    for (size_t i = 1; i < rows_; ++i) {
        if (data[i].size() != cols_) {
            throw std::invalid_argument("All rows must have the same number of columns");
        }
    }

    // Flatten to row-major storage
    data_.reserve(rows_ * cols_);
    for (const auto& row : data) {
        data_.insert(data_.end(), row.begin(), row.end());
    }
}

Matrix::Matrix(const std::vector<std::vector<mpz_class>>& data) {
    if (data.empty()) {
        rows_ = 0;
        cols_ = 0;
        return;
    }

    rows_ = data.size();
    cols_ = data[0].size();

    // Verify all rows have same length
    for (size_t i = 1; i < rows_; ++i) {
        if (data[i].size() != cols_) {
            throw std::invalid_argument("All rows must have the same number of columns");
        }
    }

    // Convert and flatten to row-major storage
    data_.reserve(rows_ * cols_);
    for (const auto& row : data) {
        for (const auto& val : row) {
            data_.emplace_back(val);
        }
    }
}

Matrix::Matrix(const std::vector<Fp>& data) : rows_(data.size()), cols_(1), data_(data) {}

Matrix::Matrix(const std::vector<mpz_class>& data) : rows_(data.size()), cols_(1) {
    data_.reserve(data.size());
    for (const auto& val : data) {
        data_.emplace_back(val);
    }
}

Matrix Matrix::identity(size_t size) {
    Matrix result(size, size);
    for (size_t i = 0; i < size; ++i) {
        result.at(i, i) = Fp::ONE;
    }
    return result;
}

Matrix Matrix::zeros(size_t rows, size_t cols) {
    return Matrix(rows, cols);
}

Matrix Matrix::random(size_t rows, size_t cols) {
    Matrix result(rows, cols);
    for (size_t i = 0; i < rows * cols; ++i) {
        result.data_[i] = Fp::random();
    }
    return result;
}

const Fp& Matrix::at(size_t row, size_t col) const {
    if (row >= rows_ || col >= cols_) {
        throw std::out_of_range("Matrix index out of range");
    }
    return data_[index(row, col)];
}

Fp& Matrix::at(size_t row, size_t col) {
    if (row >= rows_ || col >= cols_) {
        throw std::out_of_range("Matrix index out of range");
    }
    return data_[index(row, col)];
}

std::vector<Fp> Matrix::row(size_t row_idx) const {
    if (row_idx >= rows_) {
        throw std::out_of_range("Row index out of range");
    }
    std::vector<Fp> result;
    result.reserve(cols_);
    for (size_t j = 0; j < cols_; ++j) {
        result.push_back(at(row_idx, j));
    }
    return result;
}

std::vector<Fp> Matrix::col(size_t col_idx) const {
    if (col_idx >= cols_) {
        throw std::out_of_range("Column index out of range");
    }
    std::vector<Fp> result;
    result.reserve(rows_);
    for (size_t i = 0; i < rows_; ++i) {
        result.push_back(at(i, col_idx));
    }
    return result;
}

Matrix Matrix::mat_mul(const Matrix& rhs) const {
    if (cols_ != rhs.rows_) {
        throw std::invalid_argument("Matrix dimensions incompatible for multiplication: " +
                                    std::to_string(cols_) + " != " + std::to_string(rhs.rows_));
    }

    Matrix result(rows_, rhs.cols_);

    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < rhs.cols_; ++j) {
            Fp sum = Fp::ZERO;
            for (size_t k = 0; k < cols_; ++k) {
                sum = sum + at(i, k) * rhs.at(k, j);
            }
            result.at(i, j) = sum;
        }
    }

    return result;
}

Matrix Matrix::add(const Matrix& rhs, bool constant_time) const {
    if (rows_ != rhs.rows_ || cols_ != rhs.cols_) {
        throw std::invalid_argument("Matrix dimensions must match for addition");
    }

    Matrix result(rows_, cols_);

    if (constant_time) {
        size_t bin_size = ct::get_bin_size(Fp::P - 1);
        for (size_t i = 0; i < data_.size(); ++i) {
            mpz_class sum = ct::field_add(data_[i].value(), rhs.data_[i].value(), Fp::P, bin_size);
            result.data_[i] = Fp(sum);
        }
    } else {
        for (size_t i = 0; i < data_.size(); ++i) {
            result.data_[i] = data_[i] + rhs.data_[i];
        }
    }

    return result;
}

Matrix Matrix::sub(const Matrix& rhs, bool constant_time) const {
    if (rows_ != rhs.rows_ || cols_ != rhs.cols_) {
        throw std::invalid_argument("Matrix dimensions must match for subtraction");
    }

    Matrix result(rows_, cols_);

    if (constant_time) {
        size_t bin_size = ct::get_bin_size(Fp::P - 1);
        for (size_t i = 0; i < data_.size(); ++i) {
            mpz_class diff = ct::field_sub(data_[i].value(), rhs.data_[i].value(), Fp::P, bin_size);
            result.data_[i] = Fp(diff);
        }
    } else {
        for (size_t i = 0; i < data_.size(); ++i) {
            result.data_[i] = data_[i] - rhs.data_[i];
        }
    }

    return result;
}

Matrix Matrix::pow(const mpz_class& exp) const {
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i].pow(exp);
    }
    return result;
}

Matrix Matrix::pow(uint64_t exp) const {
    return pow(mpz_class(static_cast<unsigned long>(exp)));
}

Matrix Matrix::scalar_mul(const Fp& scalar) const {
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] * scalar;
    }
    return result;
}

Fp Matrix::det() const {
    if (!is_square()) {
        throw std::invalid_argument("Matrix must be square to compute determinant");
    }

    if (rows_ == 0) {
        throw std::invalid_argument("Matrix must be non-empty to compute determinant");
    }

    // Special case for 1x1
    if (rows_ == 1) {
        return at(0, 0);
    }

    // Copy data for Gaussian elimination
    std::vector<std::vector<Fp>> rows_data;
    rows_data.reserve(rows_);
    for (size_t i = 0; i < rows_; ++i) {
        rows_data.push_back(row(i));
    }

    Fp det_value = Fp::ONE;

    for (size_t col = 0; col < cols_; ++col) {
        // Partition into rows with leading zero and rows without
        std::vector<std::vector<Fp>> lz_rows;   // Leading zero rows
        std::vector<std::vector<Fp>> nlz_rows;  // Non-leading zero rows

        for (auto& r : rows_data) {
            if (r[0].is_zero()) {
                lz_rows.push_back(std::move(r));
            } else {
                nlz_rows.push_back(std::move(r));
            }
        }

        // Take pivot row
        if (nlz_rows.empty()) {
            // No pivot row means rank < n, determinant is zero
            return Fp::ZERO;
        }

        auto pivot_row = std::move(nlz_rows[0]);
        nlz_rows.erase(nlz_rows.begin());

        Fp pivot = pivot_row[0];
        det_value = det_value * pivot;

        // Precompute pivot inverse and normalize pivot row
        Fp pivot_inv = pivot.inv();
        std::vector<Fp> normalized_pivot_row;
        normalized_pivot_row.reserve(pivot_row.size());
        for (const auto& v : pivot_row) {
            normalized_pivot_row.push_back(v * pivot_inv);
        }

        // Forward elimination with normalized pivot row
        std::vector<std::vector<Fp>> nlz_rows_processed;
        for (auto& r : nlz_rows) {
            Fp lead = r[0];
            std::vector<Fp> new_row;
            new_row.reserve(r.size());
            for (size_t j = 0; j < r.size(); ++j) {
                new_row.push_back(r[j] - lead * normalized_pivot_row[j]);
            }
            nlz_rows_processed.push_back(std::move(new_row));
        }

        // Concat remaining rows and remove pivot column
        rows_data.clear();
        for (auto& r : nlz_rows_processed) {
            if (r.size() > 1) {
                rows_data.push_back(std::vector<Fp>(r.begin() + 1, r.end()));
            }
        }
        for (auto& r : lz_rows) {
            if (r.size() > 1) {
                rows_data.push_back(std::vector<Fp>(r.begin() + 1, r.end()));
            }
        }
    }

    return det_value;
}

Matrix Matrix::transpose() const {
    Matrix result(cols_, rows_);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < cols_; ++j) {
            result.at(j, i) = at(i, j);
        }
    }
    return result;
}

bool Matrix::operator==(const Matrix& rhs) const {
    if (rows_ != rhs.rows_ || cols_ != rhs.cols_) {
        return false;
    }
    return data_ == rhs.data_;
}

std::vector<Fp> Matrix::to_vector() const {
    if (cols_ != 1) {
        throw std::logic_error("to_vector() requires a column vector (cols == 1)");
    }
    return data_;
}

std::ostream& operator<<(std::ostream& os, const Matrix& mat) {
    os << "[";
    for (size_t i = 0; i < mat.rows_; ++i) {
        if (i > 0) os << " ";
        os << "[";
        for (size_t j = 0; j < mat.cols_; ++j) {
            if (j > 0) os << ", ";
            os << mat.at(i, j);
        }
        os << "]";
        if (i < mat.rows_ - 1) os << ",\n";
    }
    os << "]";
    return os;
}

Matrix to_column_vector(const std::vector<Fp>& elements) {
    return Matrix(elements);
}

Matrix to_column_vector(const std::vector<mpz_class>& elements) {
    return Matrix(elements);
}

}  // namespace rescue
