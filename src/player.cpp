/**
 * @file player.cpp
 * @brief The stochastic Simulated Annealing (SA) meta-heuristic engine.
 * * Explores the topological state space using an embarrassingly parallel OpenMP model,
 * relying on a custom thread-local Xorshift64 PRNG and fast exponential approximations.
 */

#include "../include/packingEnv.hpp"
#include <omp.h>
#include <cmath>
#include <cstring>
#include <algorithm>

// Forward declaration of the deterministic Bottom-Left dealer
int evaluatePacking(const int* sequence, const bool* rotations, int numPieces, int containerWidth);

// Instantiated exclusively in the L1 cache of each hardware thread to prevent false sharing.
thread_local uint64_t xorshift_state;

/**
 * @brief Ultra-fast, register-bound Xorshift64 generator.
 */
inline uint64_t fast_rand() {
    uint64_t x = xorshift_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return xorshift_state = x;
}

/** @brief Helper: Get random integer in range [0, max) */
inline int rand_int(int max) {
    return fast_rand() % max;
}

/** @brief Helper: Get random float in range [0.0, 1.0) */
inline float rand_float() {
    return static_cast<float>(fast_rand() & 0xFFFFFF) / 16777216.0f;
}

/**
 * @brief Fast bit-cast exponential approximation via IEEE 754 manipulation.
 * Used to bypass std::exp() latency in the Metropolis acceptance criterion.
 */
inline float fast_exp(float val) {
    float result;
    int32_t i = static_cast<int32_t>(12102203.0f * val + 1065353216.0f);
    std::memcpy(&result, &i, sizeof(float)); // Strict aliasing compliant
    return result;
}

/**
 * @brief Executes a single, thread-isolated SA trajectory.
 */
int executeSimulatedAnnealing(int numPieces, int containerWidth, int iterations) {
    
    // Allocate state vectors strictly on the stack
    int currentSeq[maxPieces];
    bool currentRot[maxPieces];
    
    int nextSeq[maxPieces];
    bool nextRot[maxPieces];
    
    // Initialize default state
    for (int i = 0; i < numPieces; ++i) {
        currentSeq[i] = i;
        currentRot[i] = false;
    }
    
    // Initial shuffle
    for (int i = numPieces - 1; i > 0; --i) {
        int j = rand_int(i + 1);
        std::swap(currentSeq[i], currentSeq[j]);
    }

    // Evaluate initial state
    int currentEnergy = evaluatePacking(currentSeq, currentRot, numPieces, containerWidth);
    int bestEnergy = currentEnergy;

    // Geometric cooling parameters
    float initialTemp = 100.0f;
    float finalTemp = 0.01f;
    float alpha = std::pow(finalTemp / initialTemp, 1.0f / static_cast<float>(iterations));
    float currentTemp = initialTemp;

    // Main exploration loop
    for (int step = 0; step < iterations; ++step) {
        
        // Clone state for mutation
        std::memcpy(nextSeq, currentSeq, numPieces * sizeof(int));
        std::memcpy(nextRot, currentRot, numPieces * sizeof(bool));

        // High-speed Mutation Loci
        if (fast_rand() & 1) {
            // Sequence Swap
            int idx1 = rand_int(numPieces);
            int idx2 = rand_int(numPieces);
            std::swap(nextSeq[idx1], nextSeq[idx2]);
        } else {
            // Orientation Flip
            int idx = rand_int(numPieces);
            nextRot[idx] = !nextRot[idx];
        }

        // Evaluate candidate
        int candidateEnergy = evaluatePacking(nextSeq, nextRot, numPieces, containerWidth);

        // Metropolis Acceptance Criterion
        int delta = candidateEnergy - currentEnergy;
        
        if (delta < 0 || rand_float() < fast_exp(-static_cast<float>(delta) / currentTemp)) {
            // Accept mutation
            std::memcpy(currentSeq, nextSeq, numPieces * sizeof(int));
            std::memcpy(currentRot, nextRot, numPieces * sizeof(bool));
            currentEnergy = candidateEnergy;
            
            // Track global best in this trajectory
            if (currentEnergy < bestEnergy) {
                bestEnergy = currentEnergy;
            }
        }

        // Geometric cooling
        currentTemp *= alpha;
    }

    return bestEnergy;
}


/**
 * @brief Deploys the SA algorithm across all available CPU cores using OpenMP.
 */
int runParallelSolver(int numPieces, int containerWidth, int totalTrajectories, int itersPerTrajectory) {
    int globalBestHeight = 999999;

    // OpenMP handles thread pooling and lock-free aggregation
    #pragma omp parallel for reduction(min:globalBestHeight)
    for (int i = 0; i < totalTrajectories; ++i) {
        
        // Unique, thread-safe seed based on execution topology
        xorshift_state = 13371337ULL + static_cast<uint64_t>(omp_get_thread_num()) + static_cast<uint64_t>(i * 1000000ULL);
        
        // Execute isolated SA
        int localBest = executeSimulatedAnnealing(numPieces, containerWidth, itersPerTrajectory);
        
        // Implicit lock-free reduction resolves this comparison safely at the end of the loop
        if (localBest < globalBestHeight) {
            globalBestHeight = localBest;
        }
    }

    return globalBestHeight;
}