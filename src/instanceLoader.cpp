/**
 * @file instanceLoader.cpp
 * @brief Initialization routines to load PDF instance data into the SoA bitboards.
 * * Extracts the specific piece dimensions for Category 1 (Prob 1 & 2) and 
 * Category 2 (Prob 1 & 2) and pre-computes their bitwise masks.
 */

#include "../include/packingEnv.hpp"
#include <vector>
#include <stdexcept>
#include <iostream>

/**
 * @brief Temporary structure to hold raw pairs before SoA injection.
 */
struct RawPiece {
    int width;
    int height;
};

/**
 * @brief Hardcoded instances extracted from the Hopper & Turton (2000) PDF.
 * Note: Due to OCR constraints, the exact pairing sequence should be double-checked 
 * against the original visual tables on pages 3 and 4 of the PDF.
 */
const std::vector<RawPiece> cat1Prob1 = {
    {2, 12}, {4, 1}, {7, 12}, {4, 5}, {8, 6}, {9, 4}, {3, 6}, {3, 5},
    {3, 5}, {3, 9}, {5, 5}, {1, 4}, {7, 4}, {14, 5}, {2, 9}, {2, 2}
};

const std::vector<RawPiece> cat1Prob2 = {
    {7, 5}, {5, 3}, {12, 5}, {3, 3}, {7, 3}, {4, 1}, {6, 5}, {5, 7},
    {5, 5}, {3, 2}, {2, 6}, {7, 2}, {6, 3}, {2, 9}, {3, 4}, {4, 2}, {3, 13}
};

const std::vector<RawPiece> cat2Prob1 = {
    {11, 3}, {11, 2}, {12, 7}, {13, 3}, {2, 3}, {7, 7}, {9, 2}, {10, 7},
    {7, 1}, {7, 2}, {8, 4}, {9, 3}, {9, 5}, {7, 3}, {7, 2}, {11, 2}, 
    {4, 1}, {13, 2}, {6, 1}, {11, 4}, {4, 5}, {5, 3}, {13, 4}, {8, 3}, {6, 3}
};

const std::vector<RawPiece> cat2Prob2 = {
    {5, 1}, {3, 12}, {11, 2}, {11, 2}, {2, 3}, {2, 5}, {5, 9}, {3, 1},
    {12, 3}, {12, 4}, {2, 3}, {6, 2}, {10, 5}, {4, 2}, {4, 4}, {6, 4},
    {11, 4}, {5, 12}, {2, 10}, {2, 16}, {1, 3}, {13, 12}, {2, 5}, {5, 4}, {3, 2}
};

/**
 * @brief Loads the selected instance data into the global SoA and pre-computes bit masks.
 * * @param category The problem category (1 or 2).
 * @param instance The problem instance (1 or 2).
 * @return int The total number of pieces loaded.
 */
int loadInstanceData(int category, int instance) {
    std::vector<RawPiece> selectedInstance;

    // Select the correct dataset based on the parameters
    if (category == 1 && instance == 1) {
        selectedInstance = cat1Prob1;
    } else if (category == 1 && instance == 2) {
        selectedInstance = cat1Prob2;
    } else if (category == 2 && instance == 1) {
        selectedInstance = cat2Prob1;
    } else if (category == 2 && instance == 2) {
        selectedInstance = cat2Prob2;
    } else {
        throw std::invalid_argument("Invalid Category or Instance specified.");
    }

    int numPieces = static_cast<int>(selectedInstance.size());

    // Inject data into the Data-Oriented SoA
    for (int i = 0; i < numPieces; ++i) {
        int w = selectedInstance[i].width;
        int h = selectedInstance[i].height;

        pieces::widths[i] = w;
        pieces::heights[i] = h;

        // 1. Generate Normal Masks (0 degree rotation)
        // Shift 1 by 'w' places and subtract 1 to get exactly 'w' consecutive 1s.
        uint64_t normalRowMask = (1ULL << w) - 1;
        for (int y = 0; y < h; ++y) {
            pieces::normalMasks[i][y] = normalRowMask;
        }

        // 2. Generate Rotated Masks (90 degree rotation)
        // Swapping width and height for the mask generation
        uint64_t rotatedRowMask = (1ULL << h) - 1;
        for (int y = 0; y < w; ++y) {
            pieces::rotatedMasks[i][y] = rotatedRowMask;
        }
    }

    return numPieces;
}