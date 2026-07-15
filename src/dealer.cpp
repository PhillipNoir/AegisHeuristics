/**
 * @file dealer.cpp
 * @brief The deterministic Bottom-Left layout evaluator using bitwise intrinsic acceleration.
 */

#include "../include/packingEnv.hpp"
#include <cstdint>

// Cross-platform compiler intrinsic for Count Trailing Zeros (CTZ)
#ifdef _MSC_VER
#include <intrin.h>
inline int fastCtz(uint64_t mask) {
    if (mask == 0) return 64;
    unsigned long index;
    _BitScanForward64(&index, mask);
    return static_cast<int>(index);
}
#else
inline int fastCtz(uint64_t mask) {
    return mask == 0 ? 64 : __builtin_ctzll(mask);
}
#endif

/**
 * @brief Evaluates a packing sequence using a strict Bottom-Left heuristic.
 * * @param sequence Array of piece indices representing the placement order.
 * @param rotations Array of booleans (false = normal, true = rotated 90 deg).
 * @param numPieces Total number of pieces to pack.
 * @param containerWidth The strict maximum width of the container (e.g., 20 or 40).
 * @return int The maximum Y-height consumed by this specific layout.
 */
int evaluatePacking(const int* sequence, const bool* rotations, int numPieces, int containerWidth) {
    
    // Allocate a cache-aligned, thread-local bitboard strictly on the stack.
    // This guarantees zero False Sharing when OpenMP scales across CPU cores.
    alignas(64) uint64_t localBoard[maxBoardHeight] = {0};
    int currentMaxHeight = 0;

    // Mask to prevent pieces from wrapping around or exceeding the container width
    const uint64_t boundsMaskTemplate = (1ULL << containerWidth) - 1;

    for (int i = 0; i < numPieces; ++i) {
        int pieceIdx = sequence[i];
        bool isRotated = rotations[i];

        // Read attributes from the Data-Oriented SoA
        int pWidth = isRotated ? pieces::heights[pieceIdx] : pieces::widths[pieceIdx];
        int pHeight = isRotated ? pieces::widths[pieceIdx] : pieces::heights[pieceIdx];

        // Base mask representation of the piece width
        uint64_t baseMask = isRotated ? pieces::rotatedMasks[pieceIdx][0] : pieces::normalMasks[pieceIdx][0];

        int bestY = -1;
        int bestX = -1;

        // Bottom-Left Scan: Move upwards from 0 to the current max peak
        for (int y = 0; y <= currentMaxHeight; ++y) {
            
            // 1. Aggregate the board terrain for the exact height of the current piece
            uint64_t aggregate = 0;
            for (int h = 0; h < pHeight; ++h) {
                aggregate |= localBoard[y + h];
            }

            // 2. Invert the aggregate to find all empty space on these layers
            uint64_t availableSpace = ~aggregate;
            
            // Limit the available space strictly to the container boundaries
            availableSpace &= boundsMaskTemplate;

            // 3. O(1) Bitwise Substring Search
            // To find 'pWidth' consecutive 1s (empty spaces), we shift and AND the mask against itself.
            uint64_t searchMask = availableSpace;
            for (int w = 1; w < pWidth; ++w) {
                searchMask &= (searchMask >> 1);
            }

            // 4. Hardware Intrinsic Evaluation
            // If searchMask is greater than 0, a valid consecutive gap exists.
            if (searchMask != 0) {
                // The hardware counts the trailing zeros to give us the absolute leftmost X coordinate
                bestX = fastCtz(searchMask);
                bestY = y;
                break; // Break early to preserve Bottom-Left strictness
            }

            // 5. Boundary Protection
            // If placing this piece exceeds our cache-aligned bitboard limit, 
            // abort and severely penalize this layout in the SA energy evaluation.
            if (bestY + pHeight >= maxBoardHeight) {
                return maxBoardHeight; // Return max penalty to force the SA to reject this state
            }
        }

        // 6. Commit the piece geometry to the local board
        uint64_t placementMask = baseMask << bestX;
        for (int h = 0; h < pHeight; ++h) {
            localBoard[bestY + h] |= placementMask;
        }

        // 6. Update the global height ceiling
        if (bestY + pHeight > currentMaxHeight) {
            currentMaxHeight = bestY + pHeight;
        }
    }

    return currentMaxHeight;
}