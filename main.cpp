#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>

using Matrix = std::vector<std::vector<int>>;
using Clock = std::chrono::steady_clock;

Matrix randomMatrix(size_t rows, size_t cols, int minVal, int maxVal, unsigned int seed) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> dist(minVal, maxVal);

  Matrix m(rows, std::vector<int>(cols));
  for (size_t r = 0; r < rows; ++r) {
    for (size_t c = 0; c < cols; ++c) {
      m[r][c] = dist(rng);
    }
  }
  return m;
}

Matrix naiveMatMul(
  const Matrix& v1,
  const Matrix& v2
) {
  const size_t i = v1.size();
  const size_t j = v1[0].size();
  const size_t k = v2[0].size();
  Matrix result(i, std::vector<int>(k, 0));

  for (size_t x = 0; x < i; ++x) {
    auto& resultRow = result[x];
    const auto& v1Row = v1[x];
    for (size_t y = 0; y < k; ++y) {
      int sum = 0;
      for (size_t z = 0; z < j; ++z) {
        sum += v1Row[z] * v2[z][y];
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
  const size_t i = v1.size();
  const size_t j = v1[0].size();
  const size_t k = v2[0].size();
  Matrix result(i, std::vector<int>(k, 0));

  for (size_t x = 0; x < i; ++x) {
    auto& resultRow = result[x];
    const auto& v1Row = v1[x];
    for (size_t z = 0; z < j; ++z) {
      const int value = v1Row[z];
      const auto& v2Row = v2[z];
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
  const size_t i = v1.size();
  const size_t j = v1[0].size();
  const size_t k = v2[0].size();
  constexpr size_t TileSize = 128;
  Matrix result(i, std::vector<int>(k, 0));

  for (size_t xx = 0; xx < i; xx += TileSize) {
    const size_t xEnd = std::min(xx + TileSize, i);

    for (size_t yy = 0; yy < k; yy += TileSize) {
        const size_t yEnd = std::min(yy + TileSize, k);
      
      for (size_t zz = 0; zz < j; zz += TileSize) {
            const size_t zEnd = std::min(zz + TileSize, j);

        for (size_t x = xx; x < xEnd; ++x) {
          auto& resultRow = result[x];
          const auto& v1Row = v1[x];
          for (size_t z = zz; z < zEnd; ++z) {
            const int value = v1Row[z];
            const auto& v2Row = v2[z];
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

void printMatrix(const Matrix& m) {
  for (const auto& row : m) {
    for (int val : row) std::cout << val << " ";
    std::cout << "\n";
  }
}

int main() {
  const size_t N = 512;

  Matrix v1 = randomMatrix(N, N, 0, 9, 42);
  Matrix v2 = randomMatrix(N, N, 0, 9, 1337);

  Matrix result1, result2, result3;

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

  // sanity check: both implementations should agree
  const bool match = (result1 == result2) && (result1 == result3);
  std::cout << "Results match: " << (match ? "yes" : "no") << "\n";

  return 0;
}