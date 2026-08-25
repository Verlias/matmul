#include "Matrix.hpp"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Hello World" << std::endl;
  Matrix<int> intMatrix(10,20);
  intMatrix.getValues();
  std::cout << std::endl;
  

  std::vector<std::vector<int>> v1 = {{1,2},{1,1}};
  std::vector<std::vector<int>> v2 = {{2,2},{2,2}};
  std::vector<int> result;

  // Matrix Multiplication Naive Implementation
  int n = v1.size();
  int k = v1[0].size();
  int m = v2[0].size();
  
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      int mult = 0;
      for (int z = 0; z < k; ++z) {
          mult += v1[i][z] * v2[z][j]; 
      }
      result.push_back(mult);
    }
  }

  std::cout << "\n";

  for (int x : result) {
    std::cout << x << " ";
  }

  std::cout << "\n";

  // Matrix Multiplication Naive Cache Friendly Implementation
  /*
  
  In naive implementation (NON-CACHE FRIENDLY)
  v1[i][z] -> z is incrementing but moving through one row
  v2[z][j] -> z is incrementing but moving through multiple rows

  This is bad for caching because each row is a contiguous block of memory, making it hard to cache
  Due to it having to pull different memory locations

  In naive implementation (Cache Friendly)
  v1[i][z] -> z is incrementing but moving through one row
  v2[z][j] -> j is incremening moving through one row as well

  Key change is that we update directly the position within the result matrix and
  Add to it as we continue to change rows
  
  v1 {
  <std::vector> 1 1
  <std::vector> 1 1
  }

  v2 {
  <std::vector> 2 2
  <std::vector> 2 2
  }
  */
  std::vector<std::vector<int>> result_2 = {{0,0}, {0,0}};

  for (int i = 0; i < n; ++i) {
    for (int z = 0; z < k; ++z) {
      for (int j = 0; j < m; ++j) {
        result_2[i][j] += v1[i][z] * v2[z][j];
      }
    }
  }
 
  std::cout << "\n";

  return 0;
}
