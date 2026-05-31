#pragma once

/**
 * Step 3 — Detection Core (unfenced integration layer)
 *
 * Connects preprocessed tokens to:
 *   - Exact match: KMP (Exact_match.cpp)
 *   - Structural match: LCS via StructuralSimilarity.cpp (fenced)
 *
 * Does NOT modify rolling-hash or LCS implementations.
 */

#include <string>
#include <vector>
#include "exact_match/Exact_Match.h"
#include "scoring/scoring.hpp"

struct DetectionResult {
    double exactSimilarity        = 0.0;
    double structuralSimilarity   = 0.0;
    std::vector<SegmentMatch> exactSegments;
};

class DetectionCore {
public:
    explicit DetectionCore(double structuralThreshold = 0.45);

    DetectionResult compare(const std::vector<std::string>& queryTokens,
                            const std::vector<std::string>& sourceTokens) const;

    std::vector<SegmentMatch> findExactSegments(
        const std::vector<std::string>& queryTokens,
        const std::vector<std::string>& sourceTokens) const;

private:
    double structuralThreshold_;
};
