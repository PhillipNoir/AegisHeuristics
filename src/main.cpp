/**
 * @file main.cpp
 * @brief Entry point for the Aegis Heuristics optimization engine.
 * * Orchestrates the loading of PDF instances, triggers the parallel Simulated Annealing
 * solver, and outputs the final optimal heights and execution times.
 */

#include "../include/packingEnv.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <omp.h>

// Forward declarations for the pipeline
int loadInstanceData(int category, int instance);
int runParallelSolver(int numPieces, int containerWidth, int totalTrajectories, int itersPerTrajectory);

/**
 * @brief Helper function to execute and time a specific problem instance.
 */
void solveInstance(int category, int instance, int containerWidth, int trajectories, int iterations) {
    std::cout << "Target: Category " << category << " | Problem " << instance << "\n";
    std::cout << "Container Width: " << containerWidth << "\n";
    
    // 1. Load data into the SoA Bitboards (Cold Path)
    int numPieces = loadInstanceData(category, instance);
    std::cout << "Loaded Pieces: " << numPieces << "\n";

    // 2. Start macro-timer
    auto startTime = std::chrono::high_resolution_clock::now();

    // 3. Fire the embarrassingly parallel stochastic engine (Hot Path)
    int bestHeight = runParallelSolver(numPieces, containerWidth, trajectories, iterations);

    // 4. Stop timer
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> executionTime = endTime - startTime;

    // 5. Output results
    std::cout << ">> OPTIMAL HEIGHT FOUND : " << bestHeight << "\n";
    std::cout << ">> EXECUTION TIME       : " << std::fixed << std::setprecision(2) 
              << executionTime.count() << " ms\n";
    std::cout << "\n";
}

int main() {
    std::cout << "      AEGIS HEURISTICS - 2D PACKING OPTIMIZER           \n";
    std::cout << "      Hardware Threads Detected: " << omp_get_max_threads() << "\n\n";

    // Tuning Parameters for Simulated Annealing
    // These dictate the breadth and depth of the state-space exploration.
    const int numTrajectories = 128;   // Number of independent OpenMP threads/walks
    const int saIterations = 50000;    // Mutations per trajectory

    // Execute Category 1
    // Max Width: 20
    solveInstance(1, 1, 20, numTrajectories, saIterations);
    solveInstance(1, 2, 20, numTrajectories, saIterations);

    // Execute Category 2
    // Max Width: 40
    solveInstance(2, 1, 40, numTrajectories, saIterations);
    solveInstance(2, 2, 40, numTrajectories, saIterations);

    return 0;
}