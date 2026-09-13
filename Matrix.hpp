#pragma once

#include <cstddef>
#include <vector>

class Matrix {
public:
    std::size_t rows;
    std::size_t cols;
    std::vector<int> values;

    Matrix()
        : rows(0),
          cols(0) {}

    Matrix(
        std::size_t rowCount,
        std::size_t columnCount,
        int initialValue = 0
    )
        : rows(rowCount),
          cols(columnCount),
          values(rowCount * columnCount, initialValue) {}

    int& operator()(std::size_t row, std::size_t col) {
        return values[row * cols + col];
    }

    const int& operator()(
        std::size_t row,
        std::size_t col
    ) const {
        return values[row * cols + col];
    }

    int* rowData(std::size_t row) {
        return values.data() + row * cols;
    }

    const int* rowData(std::size_t row) const {
        return values.data() + row * cols;
    }

    bool operator==(const Matrix& other) const {
        return rows == other.rows &&
               cols == other.cols &&
               values == other.values;
    }
};