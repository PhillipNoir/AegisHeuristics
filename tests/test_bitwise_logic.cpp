/**
 * @file test_bitwise_logic.cpp
 * @brief Google Test suite for the Aegis Heuristics deterministic Bottom-Left dealer.
 * * Isolates the bitwise manipulation and hardware intrinsic math to ensure 
 * 100% collision-free topological placements.
 */

#include <gtest/gtest.h>
#include "../include/packingEnv.hpp"

// Forward declaration of the engine's evaluator
int evaluatePacking(const int* sequence, const bool* rotations, int numPieces, int containerWidth);

/**
 * @brief Helper function to inject mock pieces directly into the SoA.
 * Bypasses the instance loader for pure, isolated unit testing.
 */
void mockPiece(int id, int w, int h) {
    pieces::widths[id] = w;
    pieces::heights[id] = h;

    uint64_t normalRowMask = (1ULL << w) - 1;
    for (int y = 0; y < h; ++y) {
        pieces::normalMasks[id][y] = normalRowMask;
    }

    uint64_t rotatedRowMask = (1ULL << h) - 1;
    for (int y = 0; y < w; ++y) {
        pieces::rotatedMasks[id][y] = rotatedRowMask;
    }
}


/**
 * @brief Tests if the bitwise scan correctly places items side-by-side 
 * without increasing the global container height.
 */
TEST(DealerTest, HorizontalStacking) {
    // Create a 4x4 piece at index 0
    mockPiece(0, 4, 4);

    // Sequence: Pack five of the same 4x4 pieces
    int seq[5] = {0, 0, 0, 0, 0};
    bool rot[5] = {false, false, false, false, false};

    // Container width is 20. 
    // 5 pieces of width 4 should fit perfectly in a single row (5 * 4 = 20).
    int finalHeight = evaluatePacking(seq, rot, 5, 20);

    // The height should remain exactly 4.
    ASSERT_EQ(finalHeight, 4) << "Horizontal bitwise gap detection failed.";
}

/**
 * @brief Tests if the bitboard correctly wraps to the next available Y-layer 
 * when the X-axis boundary mask is exceeded.
 */
TEST(DealerTest, VerticalOverflow) {
    // Create a 4x4 piece at index 1
    mockPiece(1, 4, 4);

    // Sequence: Pack SIX of the same 4x4 pieces
    int seq[6] = {1, 1, 1, 1, 1, 1};
    bool rot[6] = {false, false, false, false, false, false};

    // Container width is 20. 
    // 5 pieces fill the first row. The 6th piece MUST overflow to the top.
    int finalHeight = evaluatePacking(seq, rot, 6, 20);

    // The height should jump to 8.
    ASSERT_EQ(finalHeight, 8) << "Vertical boundary mask overflow failed.";
}

/**
 * @brief Tests the collision detection logic when pieces are intentionally 
 * rotated and forced to fit into irregular bitwise gaps.
 */
TEST(DealerTest, RotatedIrregularGap) {
    // Piece 2: 10x2 (Wide and flat)
    mockPiece(2, 10, 2);
    // Piece 3: 2x10 (Tall and thin)
    mockPiece(3, 2, 10);

    // Sequence: Pack Piece 2, then Piece 3, then Piece 2 (ROTATED)
    int seq[3] = {2, 3, 2};
    bool rot[3] = {false, false, true}; // The last piece is rotated 90 degrees!

    // Layout Logic (Container Width 20):
    // 1. Piece 2 (Normal) placed at X:0, Y:0. Consumes width 10, height 2.
    // 2. Piece 3 (Normal) placed at X:10, Y:0. Consumes width 2, height 10.
    // 3. Piece 2 (ROTATED becomes 2x10). 
    //    It will scan Y:0, find X:12 open, and place itself snugly next to Piece 3.
    
    int finalHeight = evaluatePacking(seq, rot, 3, 20);

    // The tallest pieces are height 10, and they sit on the bottom row.
    ASSERT_EQ(finalHeight, 10) << "Rotated piece SoA retrieval or gap detection failed.";
}

/**
 * @brief Validates the strict limit of the container boundary mask.
 */
TEST(DealerTest, AbsoluteBoundaryEnforcement) {
    // Piece 4: 15x5
    mockPiece(4, 15, 5);

    // Pack two of them
    int seq[2] = {4, 4};
    bool rot[2] = {false, false};

    // Container width is 20. 
    // Piece A takes X:0 to X:15.
    // Piece B (width 15) tries to fit in the remaining 5 units of space.
    // The boundsMaskTemplate in dealer.cpp MUST block it and force it upward.
    int finalHeight = evaluatePacking(seq, rot, 2, 20);

    // They must stack vertically, resulting in a height of 10.
    ASSERT_EQ(finalHeight, 10) << "Bitwise boundary masking failed to block out-of-bounds placement.";
}

/**
 * @brief Tests the "Valley" scenario to ensure pieces strictly fall to the lowest 
 * possible Y-coordinate, preventing accidental floating on higher bits.
 */
TEST(DealerTest, GravityAndValleyFill) {
    // Piece 5: 5x10 (Tall Tower)
    mockPiece(5, 5, 10);
    // Piece 6: 10x5 (Wide Valley Floor)
    mockPiece(6, 10, 5);
    // Piece 7: 4x2 (Small Block to drop into the valley)
    mockPiece(7, 4, 2);

    // Sequence: Tower -> Valley Floor -> Tower -> Small Block
    int seq[4] = {5, 6, 5, 7};
    bool rot[4] = {false, false, false, false};

    // Layout Logic (Container Width 20):
    // 1. Tower 1 (5x10) goes to X:0, Y:0
    // 2. Valley Floor (10x5) goes to X:5, Y:0
    // 3. Tower 2 (5x10) goes to X:15, Y:0
    // Now we have a U-shape. A valley exists between X:5 and X:15, resting at Y:5.
    // 4. Small Block (4x2) MUST fall perfectly into the valley at X:5, Y:5.
    
    int finalHeight = evaluatePacking(seq, rot, 4, 20);

    // If the bitwise scan works perfectly, the small block won't float on top of the towers.
    // The maximum height of the entire layout should remain exactly 10.
    ASSERT_EQ(finalHeight, 10) << "Gravity failure: The piece floated instead of falling into the valley.";
}

/**
 * @brief Fuzz testing. Forces an absolute worst-case scenario where the 
 * stochastic engine stacks everything vertically, threatening a buffer overflow.
 */
TEST(DealerTest, BufferOverflowPrevention) {
    // Piece 8: 20x20 (Massive block that fills the entire width)
    mockPiece(8, 20, 20);

    // Sequence: Pack 30 of these massive blocks.
    // Mathematical height = 30 * 20 = 600.
    // Our maxBoardHeight in packingEnv.hpp is 512.
    int seq[30];
    bool rot[30];
    for(int i = 0; i < 30; ++i) {
        seq[i] = 8;
        rot[i] = false;
    }

    // This should gracefully truncate or return the maxBoardHeight without causing a SegFault.
    int finalHeight = evaluatePacking(seq, rot, 30, 20);

    // The engine should abort the placement and return the safety limit.
    ASSERT_GE(finalHeight, 500) << "The engine failed to protect the L1 cache boundary.";
}