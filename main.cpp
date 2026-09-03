#include <iostream>
#include <vector>
#include <chrono>
#include <random>

using Matrix = std::vector<std::vector<int>>;

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
  size_t i = v1.size();
  size_t j = v1[0].size();
  size_t k = v2[0].size();
  Matrix result(i, std::vector<int>(k, 0));

  for (size_t x = 0; x < i; ++x) {
    for (size_t y = 0; y < k; ++y) {
      for (size_t z = 0; z < j; ++z) {
        result[x][y] += v1[x][z] * v2[z][y];
      }
    }
  }
  return result;
}

Matrix naiveCacheFriendly(
  const Matrix& v1,
  const Matrix& v2
) {
  size_t i = v1.size();
  size_t j = v1[0].size();
  size_t k = v2[0].size();
  Matrix result(i, std::vector<int>(k, 0));

  // loop order x, z, y is the cache-friendly one:
  // v1[x][z] is a scalar (broadcast), v2[z][y] and result[x][y]
  // are both walked row-wise (contiguous) as y increments
  for (size_t x = 0; x < i; ++x) {
    for (size_t z = 0; z < j; ++z) {
      for (size_t y = 0; y < k; ++y) {
        result[x][y] += v1[x][z] * v2[z][y];
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

  Matrix result1, result2;

  {
    auto start = std::chrono::high_resolution_clock::now();
    result1 = naiveMatMul(v1, v2);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "naiveMatMul time: " << duration.count() << " ms\n";
  }

  {
    auto start = std::chrono::high_resolution_clock::now();
    result2 = naiveCacheFriendly(v1, v2);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "naiveCacheFriendly time: " << duration.count() << " ms\n";
  }

  // sanity check: both implementations should agree
  bool match = (result1 == result2);
  std::cout << "Results match: " << (match ? "yes" : "no") << "\n";

  return 0;
}