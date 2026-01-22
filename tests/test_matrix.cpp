/**
 * @file test_matrix.cpp
 * @brief Unit tests for Matrix operations.
 */

#include <rescue/matrix.hpp>

#include <gtest/gtest.h>

using namespace rescue;

class MatrixTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 2x2 identity
        identity_2x2 = Matrix::identity(2);

        // 2x2 test matrix [[1, 2], [3, 4]]
        test_2x2 = Matrix(std::vector<std::vector<Fp>>{
            {Fp(uint64_t{1}), Fp(uint64_t{2})},
            {Fp(uint64_t{3}), Fp(uint64_t{4})}
        });

        // Column vector [1, 2, 3]
        col_vec = Matrix(std::vector<Fp>{Fp(uint64_t{1}), Fp(uint64_t{2}), Fp(uint64_t{3})});
    }

    Matrix identity_2x2;
    Matrix test_2x2;
    Matrix col_vec;
};

TEST_F(MatrixTest, Construction) {
    // Default construction
    Matrix empty;
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.rows(), 0);
    EXPECT_EQ(empty.cols(), 0);

    // From dimensions
    Matrix zeros(3, 4);
    EXPECT_EQ(zeros.rows(), 3);
    EXPECT_EQ(zeros.cols(), 4);
    EXPECT_TRUE(zeros(0, 0).is_zero());

    // From 2D vector of Fp
    EXPECT_EQ(test_2x2.rows(), 2);
    EXPECT_EQ(test_2x2.cols(), 2);
    EXPECT_EQ(test_2x2(0, 0), Fp(uint64_t{1}));
    EXPECT_EQ(test_2x2(1, 1), Fp(uint64_t{4}));

    // From 1D vector (column vector)
    EXPECT_EQ(col_vec.rows(), 3);
    EXPECT_EQ(col_vec.cols(), 1);
}

TEST_F(MatrixTest, Identity) {
    EXPECT_EQ(identity_2x2(0, 0), Fp::ONE);
    EXPECT_EQ(identity_2x2(0, 1), Fp::ZERO);
    EXPECT_EQ(identity_2x2(1, 0), Fp::ZERO);
    EXPECT_EQ(identity_2x2(1, 1), Fp::ONE);

    // Larger identity
    auto id_5 = Matrix::identity(5);
    for (size_t i = 0; i < 5; ++i) {
        for (size_t j = 0; j < 5; ++j) {
            if (i == j) {
                EXPECT_EQ(id_5(i, j), Fp::ONE);
            } else {
                EXPECT_TRUE(id_5(i, j).is_zero());
            }
        }
    }
}

TEST_F(MatrixTest, ElementAccess) {
    EXPECT_EQ(test_2x2.at(0, 0), Fp(uint64_t{1}));
    EXPECT_EQ(test_2x2.at(0, 1), Fp(uint64_t{2}));
    EXPECT_EQ(test_2x2.at(1, 0), Fp(uint64_t{3}));
    EXPECT_EQ(test_2x2.at(1, 1), Fp(uint64_t{4}));

    // Out of range should throw
    EXPECT_THROW(test_2x2.at(2, 0), std::out_of_range);
    EXPECT_THROW(test_2x2.at(0, 2), std::out_of_range);
}

TEST_F(MatrixTest, RowColumn) {
    auto row0 = test_2x2.row(0);
    EXPECT_EQ(row0.size(), 2);
    EXPECT_EQ(row0[0], Fp(uint64_t{1}));
    EXPECT_EQ(row0[1], Fp(uint64_t{2}));

    auto col0 = test_2x2.col(0);
    EXPECT_EQ(col0.size(), 2);
    EXPECT_EQ(col0[0], Fp(uint64_t{1}));
    EXPECT_EQ(col0[1], Fp(uint64_t{3}));
}

TEST_F(MatrixTest, MatMul) {
    // A * I = A
    auto result = test_2x2.mat_mul(identity_2x2);
    EXPECT_EQ(result, test_2x2);

    // I * A = A
    result = identity_2x2.mat_mul(test_2x2);
    EXPECT_EQ(result, test_2x2);

    // [[1, 2], [3, 4]] * [[1, 2], [3, 4]] = [[7, 10], [15, 22]]
    result = test_2x2.mat_mul(test_2x2);
    EXPECT_EQ(result(0, 0), Fp(uint64_t{7}));
    EXPECT_EQ(result(0, 1), Fp(uint64_t{10}));
    EXPECT_EQ(result(1, 0), Fp(uint64_t{15}));
    EXPECT_EQ(result(1, 1), Fp(uint64_t{22}));

    // Dimension mismatch should throw
    EXPECT_THROW(test_2x2.mat_mul(col_vec), std::invalid_argument);
}

TEST_F(MatrixTest, Add) {
    // A + 0 = A
    auto zeros = Matrix::zeros(2, 2);
    auto result = test_2x2.add(zeros);
    EXPECT_EQ(result, test_2x2);

    // A + A = 2A
    result = test_2x2.add(test_2x2);
    EXPECT_EQ(result(0, 0), Fp(uint64_t{2}));
    EXPECT_EQ(result(1, 1), Fp(uint64_t{8}));

    // Dimension mismatch should throw
    EXPECT_THROW(test_2x2.add(col_vec), std::invalid_argument);
}

TEST_F(MatrixTest, Sub) {
    // A - A = 0
    auto result = test_2x2.sub(test_2x2);
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            EXPECT_TRUE(result(i, j).is_zero());
        }
    }

    // A - 0 = A
    auto zeros = Matrix::zeros(2, 2);
    result = test_2x2.sub(zeros);
    EXPECT_EQ(result, test_2x2);
}

TEST_F(MatrixTest, Pow) {
    // Element-wise power
    auto result = test_2x2.pow(uint64_t{2});
    EXPECT_EQ(result(0, 0), Fp(uint64_t{1}));   // 1^2
    EXPECT_EQ(result(0, 1), Fp(uint64_t{4}));   // 2^2
    EXPECT_EQ(result(1, 0), Fp(uint64_t{9}));   // 3^2
    EXPECT_EQ(result(1, 1), Fp(uint64_t{16}));  // 4^2

    // Power of 0 = 1
    result = test_2x2.pow(uint64_t{0});
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            EXPECT_EQ(result(i, j), Fp::ONE);
        }
    }
}

TEST_F(MatrixTest, ScalarMul) {
    auto result = test_2x2.scalar_mul(Fp(uint64_t{2}));
    EXPECT_EQ(result(0, 0), Fp(uint64_t{2}));
    EXPECT_EQ(result(0, 1), Fp(uint64_t{4}));
    EXPECT_EQ(result(1, 0), Fp(uint64_t{6}));
    EXPECT_EQ(result(1, 1), Fp(uint64_t{8}));
}

TEST_F(MatrixTest, Determinant) {
    // det([[1, 2], [3, 4]]) = 1*4 - 2*3 = -2 = p - 2
    Fp det = test_2x2.det();
    EXPECT_EQ(det, Fp(Fp::P - 2));

    // det(I) = 1
    EXPECT_EQ(identity_2x2.det(), Fp::ONE);

    // det of singular matrix = 0
    Matrix singular(std::vector<std::vector<Fp>>{
        {Fp(uint64_t{1}), Fp(uint64_t{2})},
        {Fp(uint64_t{2}), Fp(uint64_t{4})}
    });
    EXPECT_TRUE(singular.det().is_zero());

    // Non-square should throw
    Matrix non_square(2, 3);
    EXPECT_THROW(non_square.det(), std::invalid_argument);
}

TEST_F(MatrixTest, Transpose) {
    auto transposed = test_2x2.transpose();
    EXPECT_EQ(transposed(0, 0), test_2x2(0, 0));
    EXPECT_EQ(transposed(0, 1), test_2x2(1, 0));
    EXPECT_EQ(transposed(1, 0), test_2x2(0, 1));
    EXPECT_EQ(transposed(1, 1), test_2x2(1, 1));

    // Transpose of column vector is row vector
    auto col_transposed = col_vec.transpose();
    EXPECT_EQ(col_transposed.rows(), 1);
    EXPECT_EQ(col_transposed.cols(), 3);
}

TEST_F(MatrixTest, ToVector) {
    auto vec = col_vec.to_vector();
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], Fp(uint64_t{1}));
    EXPECT_EQ(vec[1], Fp(uint64_t{2}));
    EXPECT_EQ(vec[2], Fp(uint64_t{3}));

    // Non-column vector should throw
    EXPECT_THROW(test_2x2.to_vector(), std::logic_error);
}

TEST_F(MatrixTest, Random) {
    auto random_mat = Matrix::random(3, 4);
    EXPECT_EQ(random_mat.rows(), 3);
    EXPECT_EQ(random_mat.cols(), 4);

    // Check values are in range
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            EXPECT_GE(random_mat(i, j).value(), 0);
            EXPECT_LT(random_mat(i, j).value(), Fp::P);
        }
    }
}

TEST_F(MatrixTest, OperatorOverloads) {
    // +
    EXPECT_EQ(test_2x2 + identity_2x2, test_2x2.add(identity_2x2));

    // -
    EXPECT_EQ(test_2x2 - identity_2x2, test_2x2.sub(identity_2x2));

    // *
    EXPECT_EQ(test_2x2 * identity_2x2, test_2x2.mat_mul(identity_2x2));

    // * (scalar)
    EXPECT_EQ(test_2x2 * Fp(uint64_t{2}), test_2x2.scalar_mul(Fp(uint64_t{2})));
}

TEST_F(MatrixTest, ConstantTimeOperations) {
    // Test constant-time addition
    auto result_ct = test_2x2.add(identity_2x2, true);
    auto result_normal = test_2x2.add(identity_2x2, false);
    EXPECT_EQ(result_ct, result_normal);

    // Test constant-time subtraction
    result_ct = test_2x2.sub(identity_2x2, true);
    result_normal = test_2x2.sub(identity_2x2, false);
    EXPECT_EQ(result_ct, result_normal);
}
