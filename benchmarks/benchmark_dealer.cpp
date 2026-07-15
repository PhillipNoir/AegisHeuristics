/**
 * @file benchmark_dealer.cpp
 * @brief Nanosecond-level micro-benchmarking for the deterministic Bottom-Left evaluator.
 */

#include <benchmark/benchmark.h>
#include "../include/packingEnv.hpp"

// Forward declaration of the engine's evaluator
int evaluatePacking(const int* sequence, const bool* rotations, int numPieces, int containerWidth);

/**
 * @brief Helper function to inject mock pieces directly into the SoA for benchmarking.
 */
void setupBenchmarkEnvironment() {
    // Populate the SoA with a mix of small and large pieces to simulate 
    // a real Category 2 load (container width 40).
    for (int i = 0; i < maxPieces; ++i) {
        int w = (i % 15) + 2; // Widths between 2 and 16
        int h = (i % 10) + 2; // Heights between 2 and 11
        
        pieces::widths[i] = w;
        pieces::heights[i] = h;

        uint64_t normalRowMask = (1ULL << w) - 1;
        for (int y = 0; y < h; ++y) {
            pieces::normalMasks[i][y] = normalRowMask;
        }

        uint64_t rotatedRowMask = (1ULL << h) - 1;
        for (int y = 0; y < w; ++y) {
            pieces::rotatedMasks[i][y] = rotatedRowMask;
        }
    }
}

/**
 * @brief Micro-benchmark targeting the O(1) bitwise collision detection.
 */
static void BM_BitwiseBottomLeft_Cat2(benchmark::State& state) {
    // Initialize the static SoA exactly once before the timer starts
    setupBenchmarkEnvironment();

    // Create a static Worst-Case sequence for Category 2 (25 items)
    int seq[25];
    bool rot[25];
    for(int i = 0; i < 25; ++i) {
        seq[i] = i;           // Every piece is unique
        rot[i] = (i % 2 == 0); // Alternate rotations to force branching evaluation
    }

    // The Google Benchmark loop: This runs the target code thousands of times
    // to achieve statistical significance.
    for (auto _ : state) {
        // Execute the Hot Path
        int finalHeight = evaluatePacking(seq, rot, 25, 40);
        
        // Prevent the -O3 compiler flag from optimizing the loop away entirely
        benchmark::DoNotOptimize(finalHeight);
        
        // Clobber memory barriers to force the CPU to read/write real L1 cache
        benchmark::ClobberMemory();
    }
}

// Register the benchmark and force the output to display in nanoseconds
BENCHMARK(BM_BitwiseBottomLeft_Cat2)->Unit(benchmark::kNanosecond);