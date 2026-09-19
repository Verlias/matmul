#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

#include "Matrix.hpp"

using Clock = std::chrono::steady_clock;
using std::size_t;

void checkDimensions(const Matrix& left, const Matrix& right) {
    if (left.rows == 0 || left.cols == 0 || right.rows == 0 || right.cols == 0 ||
        left.cols != right.rows) {
        throw std::invalid_argument("matrix dimensions must be positive and compatible");
    }
}

Matrix randomMatrix(
    size_t rows,
    size_t cols,
    int minValue,
    int maxValue,
    unsigned int seed
) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> distribution(
        minValue,
        maxValue
    );

    Matrix matrix(rows, cols);

    for (size_t row = 0; row < rows; ++row) {
        int* currentRow = matrix.rowData(row);

        for (size_t col = 0; col < cols; ++col) {
            currentRow[col] = distribution(rng);
        }
    }

    return matrix;
}

Matrix naiveMatMul(
    const Matrix& v1,
    const Matrix& v2
) {
    checkDimensions(v1, v2);
    Matrix result(v1.rows, v2.cols);

    for (size_t x = 0; x < v1.rows; ++x) {
        int* resultRow = result.rowData(x);
        const int* v1Row = v1.rowData(x);
        for (size_t y = 0; y < v2.cols; ++y) {
            int sum = 0;
            for (size_t z = 0; z < v1.cols; ++z) {
                sum += v1Row[z] * v2(z, y);
            }
            resultRow[y] = sum;
        }
    }
    return result;
}

Matrix naiveCacheFriendly(
    const Matrix& v1,
    const Matrix& v2
) {
    checkDimensions(v1, v2);
    Matrix result(v1.rows, v2.cols);

    for (size_t x = 0; x < v1.rows; ++x) {
        int* resultRow = result.rowData(x);
        const int* v1Row = v1.rowData(x);
        for (size_t z = 0; z < v1.cols; ++z) {
            const int value = v1Row[z];
            const int* v2Row = v2.rowData(z);
            for (size_t y = 0; y < v2.cols; ++y) {
                resultRow[y] += value * v2Row[y];
            }
        }
    }
    return result;
}

Matrix cacheTiling(
    const Matrix& v1,
    const Matrix& v2
) {
    checkDimensions(v1, v2);
    constexpr size_t TileSize = 128;
    Matrix result(v1.rows, v2.cols);

    for (size_t xx = 0; xx < v1.rows; xx += TileSize) {
        const size_t xEnd = std::min(xx + TileSize, v1.rows);
        for (size_t yy = 0; yy < v2.cols; yy += TileSize) {
            const size_t yEnd = std::min(yy + TileSize, v2.cols);
            for (size_t zz = 0; zz < v1.cols; zz += TileSize) {
                const size_t zEnd = std::min(zz + TileSize, v1.cols);
                for (size_t x = xx; x < xEnd; ++x) {
                    int* resultRow = result.rowData(x);
                    const int* v1Row = v1.rowData(x);
                    for (size_t z = zz; z < zEnd; ++z) {
                        const int value = v1Row[z];
                        const int* v2Row = v2.rowData(z);
                        for (size_t y = yy; y < yEnd; ++y) {
                            resultRow[y] += value * v2Row[y];
                        }
                    }
                }
            }
        }
    }
    return result;
}

Matrix registerTiling(
    const Matrix& v1,
    const Matrix& v2
) {
    checkDimensions(v1, v2);
    const size_t i = v1.rows;
    const size_t j = v1.cols;
    const size_t k = v2.cols;

    constexpr size_t TileSize = 128;

    Matrix result(i, k, 0);

    for (size_t xx = 0; xx < i; xx += TileSize) {
        const size_t xEnd = std::min(xx + TileSize, i);

        for (size_t zz = 0; zz < j; zz += TileSize) {
            const size_t zEnd = std::min(zz + TileSize, j);

            for (size_t yy = 0; yy < k; yy += TileSize) {
                const size_t yEnd = std::min(yy + TileSize, k);

                // Largest region that fits complete 4x4 tiles.
                const size_t xFullEnd =
                    xx + ((xEnd - xx) / 4) * 4;

                const size_t yFullEnd =
                    yy + ((yEnd - yy) / 4) * 4;

                for (size_t x = xx; x < xFullEnd; x += 4) {

                    const int* v1Row0 = v1.rowData(x);
                    const int* v1Row1 = v1.rowData(x + 1);
                    const int* v1Row2 = v1.rowData(x + 2);
                    const int* v1Row3 = v1.rowData(x + 3);

                    int* resultRow0 = result.rowData(x);
                    int* resultRow1 = result.rowData(x + 1);
                    int* resultRow2 = result.rowData(x + 2);
                    int* resultRow3 = result.rowData(x + 3);

                    for (size_t y = yy; y < yFullEnd; y += 4) {

                        // 16 C accumulators
                        int c00 = resultRow0[y];
                        int c01 = resultRow0[y + 1];
                        int c02 = resultRow0[y + 2];
                        int c03 = resultRow0[y + 3];

                        int c10 = resultRow1[y];
                        int c11 = resultRow1[y + 1];
                        int c12 = resultRow1[y + 2];
                        int c13 = resultRow1[y + 3];

                        int c20 = resultRow2[y];
                        int c21 = resultRow2[y + 1];
                        int c22 = resultRow2[y + 2];
                        int c23 = resultRow2[y + 3];

                        int c30 = resultRow3[y];
                        int c31 = resultRow3[y + 1];
                        int c32 = resultRow3[y + 2];
                        int c33 = resultRow3[y + 3];

                        for (size_t z = zz; z < zEnd; ++z) {

                            // 4 A values
                            const int a0 = v1Row0[z];
                            const int a1 = v1Row1[z];
                            const int a2 = v1Row2[z];
                            const int a3 = v1Row3[z];

                            // 4 contiguous B values
                            const int* v2Row = v2.rowData(z);

                            const int b0 = v2Row[y];
                            const int b1 = v2Row[y + 1];
                            const int b2 = v2Row[y + 2];
                            const int b3 = v2Row[y + 3];

                            // Row 0
                            c00 += a0 * b0;
                            c01 += a0 * b1;
                            c02 += a0 * b2;
                            c03 += a0 * b3;

                            // Row 1
                            c10 += a1 * b0;
                            c11 += a1 * b1;
                            c12 += a1 * b2;
                            c13 += a1 * b3;

                            // Row 2
                            c20 += a2 * b0;
                            c21 += a2 * b1;
                            c22 += a2 * b2;
                            c23 += a2 * b3;

                            // Row 3
                            c30 += a3 * b0;
                            c31 += a3 * b1;
                            c32 += a3 * b2;
                            c33 += a3 * b3;
                        }

                        // Store the 4x4 C tile
                        resultRow0[y]     = c00;
                        resultRow0[y + 1] = c01;
                        resultRow0[y + 2] = c02;
                        resultRow0[y + 3] = c03;

                        resultRow1[y]     = c10;
                        resultRow1[y + 1] = c11;
                        resultRow1[y + 2] = c12;
                        resultRow1[y + 3] = c13;

                        resultRow2[y]     = c20;
                        resultRow2[y + 1] = c21;
                        resultRow2[y + 2] = c22;
                        resultRow2[y + 3] = c23;

                        resultRow3[y]     = c30;
                        resultRow3[y + 1] = c31;
                        resultRow3[y + 2] = c32;
                        resultRow3[y + 3] = c33;
                    }
                }

                for (size_t x = xx; x < xFullEnd; ++x) {

                    const int* v1Row = v1.rowData(x);
                    int* resultRow = result.rowData(x);

                    for (size_t z = zz; z < zEnd; ++z) {

                        const int a = v1Row[z];
                        const int* v2Row = v2.rowData(z);

                        for (size_t y = yFullEnd; y < yEnd; ++y) {
                            resultRow[y] += a * v2Row[y];
                        }
                    }
                }

                for (size_t x = xFullEnd; x < xEnd; ++x) {

                    const int* v1Row = v1.rowData(x);
                    int* resultRow = result.rowData(x);

                    for (size_t z = zz; z < zEnd; ++z) {

                        const int a = v1Row[z];
                        const int* v2Row = v2.rowData(z);

                        for (size_t y = yy; y < yEnd; ++y) {
                            resultRow[y] += a * v2Row[y];
                        }
                    }
                }
            }
        }
    }

    return result;
}


using Multiply = Matrix (*)(const Matrix&, const Matrix&);

struct Implementation {
    const char* name;
    Multiply multiply;
};

constexpr Implementation implementations[] = {
    {"naiveMatMul", naiveMatMul},
    {"naiveCacheFriendly", naiveCacheFriendly},
    {"cacheTiling", cacheTiling},
    {"registerTiling", registerTiling},
};

bool checkCase(size_t rows, size_t inner, size_t cols) {
    const Matrix left = randomMatrix(rows, inner, 0, 9, 42);
    const Matrix right = randomMatrix(inner, cols, 0, 9, 1337);
    const Matrix expected = naiveMatMul(left, right);

    for (const auto& implementation : implementations) {
        if (!(implementation.multiply(left, right) == expected)) {
            std::cerr << implementation.name << " failed for "
                      << rows << 'x' << inner << " * " << inner << 'x' << cols << '\n';
            return false;
        }
    }
    return true;
}

bool runChecks() {
    Matrix left(2, 3);
    left.values = {1, 2, 3, 4, 5, 6};
    Matrix right(3, 2);
    right.values = {7, 8, 9, 10, 11, 12};
    Matrix expected(2, 2);
    expected.values = {58, 64, 139, 154};
    for (const auto& implementation : implementations) {
        if (!(implementation.multiply(left, right) == expected)) {
            std::cerr << implementation.name << " failed the known result\n";
            return false;
        }
    }

    for (const auto& dimensions : {
             std::array<size_t, 3>{1, 1, 1},
             std::array<size_t, 3>{3, 5, 7},
             std::array<size_t, 3>{5, 129, 6},
             std::array<size_t, 3>{129, 5, 131},
             std::array<size_t, 3>{131, 129, 133},
         }) {
        if (!checkCase(dimensions[0], dimensions[1], dimensions[2])) {
            return false;
        }
    }

    for (const auto& implementation : implementations) {
        try {
            implementation.multiply(Matrix(2, 3), Matrix(4, 2));
            std::cerr << implementation.name << " accepted mismatched dimensions\n";
            return false;
        } catch (const std::invalid_argument&) {
            // Expected for every implementation.
        }
        try {
            implementation.multiply(Matrix(1, 0), Matrix(0, 1));
            std::cerr << implementation.name << " accepted an empty dimension\n";
            return false;
        } catch (const std::invalid_argument&) {
            // Expected for every implementation.
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--check") {
        const bool passed = runChecks();
        std::cout << "Checks: " << (passed ? "passed" : "failed") << '\n';
        return passed ? 0 : 1;
    }

    size_t size = 1024;
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [positive-size | --check]\n";
        return 1;
    }
    if (argc == 2) {
        try {
            const std::string argument(argv[1]);
            if (argument.empty() || argument.find_first_not_of("0123456789") != std::string::npos) {
                throw std::invalid_argument("invalid size");
            }
            size_t parsed = 0;
            size = std::stoull(argument, &parsed);
            if (parsed != argument.size() || size == 0) {
                throw std::invalid_argument("invalid size");
            }
        } catch (const std::exception&) {
            std::cerr << "Size must be a positive integer\n";
            return 1;
        }
    }

    const Matrix left = randomMatrix(size, size, 0, 9, 42);
    const Matrix right = randomMatrix(size, size, 0, 9, 1337);
    Matrix reference;

    for (const auto& implementation : implementations) {
        const auto start = Clock::now();
        Matrix result = implementation.multiply(left, right);
        const auto stop = Clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << implementation.name << " time: " << elapsed.count() << " μs\n";

        if (implementation.multiply == naiveMatMul) {
            reference = std::move(result);
        } else if (!(result == reference)) {
            std::cerr << "Results match: no (" << implementation.name << ")\n";
            return 1;
        }
    }

    std::cout << "Results match: yes\n";
    return 0;
}
