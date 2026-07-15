/**
 * @file packingEnv.hpp
 * @author Sergio "Phillip Noir" Gonzalez
 * @date 2026-07-12
 * @brief Data-Oriented Design (DOD) environment for the 2D rectangular packing problem.
 * * This header defines the cache-aligned bitboards and the Structure of Arrays (SoA)
 * used to represent the piece geometries and the packing container. By avoiding AoS
 * (Array of Structures), we maximize L1 cache hits and eliminate branch mispredictions.
 */

#pragma once
#include <cstdint>

/**
 * @defgroup ConfigurationConstants Compile-Time Limits
 * @brief Hardcoded constraints based on Category 1 and Category 2 instances to maximize performance.
 * @{
 */
constexpr int maxPieces = 32;       //Safe upper bound for pieces (Cat 2 has a max of 25 items).
constexpr int maxPieceHeight = 64;  //Maximum possible height of a single piece.
constexpr int maxBoardHeight = 512; //Safe upper bound for the container height during poor stochastic layouts.
/** @} */

/**
 * @brief The cache-aligned packing container (Bitboard).
 * * Represented as a 1D array of unsigned 64-bit integers. Each index is a Y-coordinate (row), 
 * and the bits represent the X-coordinates (columns). 
 * * The alignas(64) specifier ensures the array falls perfectly on x86 cache line boundaries. 
 * A single memory fetch will stream exactly 8 rows directly into the processor's fastest memory layer.
 */
alignas(64) inline uint64_t bitboard[maxBoardHeight] = {0};

/**
 * @namespace pieces
 * @brief Structure of Arrays (SoA) for piece attributes.
 * * Decoupling the physical dimensions from the pre-computed bit masks prevents cache pollution.
 * During the Simulated Annealing evaluation phase, the algorithm will ONLY stream the masks,
 * completely ignoring the integers that would otherwise waste cache space in a traditional OOP struct.
 */
namespace pieces {
    
    /** @brief Original widths of the pieces. */
    inline int widths[maxPieces];
    
    /** @brief Original heights of the pieces. */
    inline int heights[maxPieces];
    
    /** * @brief Pre-computed bit masks for the pieces in their default orientation.
     * @details Accessed via normalMasks[pieceIndex][rowOffset].
     */
    inline uint64_t normalMasks[maxPieces][maxPieceHeight];
    
    /** * @brief Pre-computed bit masks for the pieces rotated 90 degrees.
     * @details Accessed via rotatedMasks[pieceIndex][rowOffset].
     */
    inline uint64_t rotatedMasks[maxPieces][maxPieceHeight];

} // namespace pieces