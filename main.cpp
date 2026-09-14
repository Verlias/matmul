#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>

#include "Matrix.hpp"

using Clock = std::chrono::steady_clock;


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
  const size_t i = v1.rows;
  const size_t j = v1.cols;
  const size_t k = v2.cols;
  Matrix result(i, k, 0);

  for (size_t x = 0; x < i; ++x) {
    int* resultRow = result.rowData(x);
    const int* v1Row = v1.rowData(x);
    for (size_t y = 0; y < k; ++y) {
      int sum = 0;
      for (size_t z = 0; z < j; ++z) {
        sum += v1Row[z] * v2(z,y);
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
  const size_t i = v1.rows;
  const size_t j = v1.cols;
  const size_t k = v2.cols;
  Matrix result(i, k, 0);

  for (size_t x = 0; x < i; ++x) {
    int* resultRow = result.rowData(x);
    const int* v1Row = v1.rowData(x);
    for (size_t z = 0; z < j; ++z) {
      const int value = v1Row[z];
      const int* v2Row = v2.rowData(z);
      for (size_t y = 0; y < k; ++y) {
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
  const size_t i = v1.rows;
  const size_t j = v1.cols;
  const size_t k = v2.cols;
  constexpr size_t TileSize = 128;
  Matrix result(i, k, 0);

  for (size_t xx = 0; xx < i; xx += TileSize) {
    const size_t xEnd = std::min(xx + TileSize, i);

    for (size_t yy = 0; yy < k; yy += TileSize) {
        const size_t yEnd = std::min(yy + TileSize, k);
      
      for (size_t zz = 0; zz < j; zz += TileSize) {
            const size_t zEnd = std::min(zz + TileSize, j);

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

// Most Effective when paired with cache tiling 
Matrix registerTiling(
    const Matrix& v1,
    const Matrix& v2
) {

    const size_t rows = v1.rows;
    const size_t inner = v1.cols;
    const size_t cols = v2.cols;

    Matrix result(rows, cols, 0);

    for (size_t x = 0; x < rows; x += 2) {

        const int* v1Row0 = v1.rowData(x);
        const int* v1Row1 = v1.rowData(x + 1);

        int* resultRow0 = result.rowData(x);
        int* resultRow1 = result.rowData(x + 1);

        for (size_t y = 0; y < cols; y += 2) {

            int c00 = 0;
            int c01 = 0;
            int c10 = 0;
            int c11 = 0;

            for (size_t z = 0; z < inner; ++z) {

                int a0 = v1Row0[z];
                int a1 = v1Row1[z];

                const int* v2Row = v2.rowData(z);

                int b0 = v2Row[y];
                int b1 = v2Row[y + 1];

                c00 += a0 * b0;
                c01 += a0 * b1;

                c10 += a1 * b0;
                c11 += a1 * b1;
            }

            resultRow0[y]     = c00;
            resultRow0[y + 1] = c01;

            resultRow1[y]     = c10;
            resultRow1[y + 1] = c11;
        }
    }

    return result;
}

// void printMatrix(const Matrix& m) {
//   for (const auto& row : m) {
//     for (int val : row) std::cout << val << " ";
//     std::cout << "\n";
//   }
// }

int main() {
  const size_t N = 1096;

  Matrix v1 = randomMatrix(N, N, 0, 9, 42);
  Matrix v2 = randomMatrix(N, N, 0, 9, 1337);

  Matrix result1, result2, result3, result4;

  {
    auto start = Clock::now();
    result1 = naiveMatMul(v1, v2);
    auto stop = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "naiveMatMul time: " << duration.count() << " μs\n";
  }

  {
    auto start = Clock::now();
    result2 = naiveCacheFriendly(v1, v2);
    auto stop = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "naiveCacheFriendly time: " << duration.count() << " μs\n";
  }

  {
    auto start = Clock::now();
    result3 = cacheTiling(v1, v2);
    auto stop = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "cacheTiling time: " << duration.count() << " μs\n";
  }

  {
    auto start = Clock::now();
    result4 = registerTiling(v1, v2);
    auto stop = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "registerTiling time: " << duration.count() << " μs\n";
  }

  // sanity check: both implementations should agree
  const bool match = (result1 == result2) && (result1 == result3) && (result1 == result4);
  std::cout << "Results match: " << (match ? "yes" : "no") << "\n";

  return 0;
}