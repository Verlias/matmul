#include <iostream>
using namespace std;

template <typename T> class Matrix {
  public:
    T x;
    T y;
    
    // Constructor
    Matrix(T val1, T val2) : x(val1), y(val2) {
    }

    // Method to get Values
    void getValues() {
      cout << x << " " << y;
    }
};
