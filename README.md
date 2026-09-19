# Matrix multiplication

A small C++17 benchmark comparing four single threaded integer matrix multiplication loops. `Matrix` stores row major `int` values. Every implementation computes `A (m × n) * B (n × p)` and returns an `m × p` matrix.

## Build and run

Requires a C++17 compiler and `make`.

```sh
make                 # build ./program
make check           # correctness checks, including rectangular and tile edge cases
make run             # benchmark 1024 × 1024 matrices
./program 512        # benchmark a different square size
```

The benchmark uses fixed random seeds and values from 0 to 9. It prints each multiplication time in microseconds, then checks every result against the direct implementation. A mismatch returns a nonzero exit code. `make clean` removes the executable.

## Implementations

| Name | Loop strategy |
| --- | --- |
| `naiveMatMul` | Direct row, column, inner product (`x, y, z`). Reading a column of `B` jumps through row major memory. |
| `naiveCacheFriendly` | `x, z, y` order. The inner loop walks contiguous values in a row of `B` and the output. |
| `cacheTiling` | 128 element tiles across all three dimensions, with the same contiguous inner loop. |
| `registerTiling` | 128 element outer tiles and a 4 × 4 output block held in local accumulators; scalar loops handle incomplete edge tiles. |

All four algorithms use O(mnp) arithmetic. The tiling variants change data access and reuse rather than asymptotic work.

## Performance

Measured on a MacBook Pro with an Apple M5 Pro, macOS 26.6.2, and Apple clang 21.0.0. Build command: `g++ -O3 -march=native -std=c++17 -Wall -Wextra -Wpedantic src/main.cpp -o /tmp/matmul-clean`. Inputs were two 1024 × 1024 matrices. These are medians of five consecutive runs, in the fixed order shown. Matrix creation and result comparison were outside the timed region; result allocation and zero initialization were inside it.

| Implementation | Median time | Speedup vs. direct |
| --- | ---: | ---: |
| `naiveMatMul` | 1,055.3 ms | 1.0× |
| `naiveCacheFriendly` | 58.5 ms | 18.0× |
| `cacheTiling` | 74.6 ms | 14.1× |
| `registerTiling` | 47.7 ms | 22.1× |

These numbers describe this compiler, machine, input size, and run order. They are not a general ranking for other shapes or hardware. Rerun `./program` on your own machine when changing a kernel.

## What I learned

- Loop order had the largest effect here. Walking rows of `B` and the result contiguously reduced the median from about 1.06 seconds to 58.5 ms.
- A tile size is a parameter to measure, not an automatic improvement. The 128 element cache tiled version took about 74.6 ms here, slower than the simple contiguous loop.
- Holding a 4 × 4 result block in local accumulators gave the best measured time, about 47.7 ms. The edge loops matter: `make check` covers dimensions that are not multiples of 4 or 128.
- A performance result needs context. Compiler flags, allocation inside the timed region, input shape, and measurement order can all change the comparison.

## Where I stopped

I decided not to go deeper into the remaining possible optimizations for this project. If I continued, I would explore multithreading, explicit SIMD instructions, packing matrix data for better reuse, and tuning tile sizes for different shapes and machines. Each would need its own correctness checks and benchmarks.

## Notes and limits

- The benchmark is single threaded and uses scalar C++ code; the compiler may generate vector instructions. It does not compare against a BLAS library.
- The command line benchmark uses square matrices. `make check` also verifies rectangular products, a known result, invalid dimensions, and tile boundaries. Multiplication requires positive, compatible dimensions.
- Inputs are small nonnegative integers so 1024 element inner products fit in `int`. Arbitrary values or much larger inner dimensions can overflow signed `int`.
- `Matrix` exposes its dimensions and storage directly and does not perform bounds checks. Callers should keep its storage consistent with its dimensions.
