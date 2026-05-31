/**
 * ContentGuard — Step 3 Detection Core
 * KMP exact match (Exact_match.cpp) + fenced LCS (StructuralSimilarity.cpp)
 */

#include "pipeline/detection_core.hpp"

#include <algorithm>

#include "structuralSimilarity/Structural_Similarity.h"

DetectionCore::DetectionCore(double structuralThreshold)
    : structuralThreshold_(structuralThreshold) {}

DetectionResult DetectionCore::compare(
    const std::vector<std::string>& queryTokens,
    const std::vector<std::string>& sourceTokens) const {

    DetectionResult result;
    result.exactSimilarity =
        computeExactTokenSimilarity(queryTokens, sourceTokens);

    StructuralSimilarity structuralMatcher(structuralThreshold_);
    result.structuralSimilarity =
        structuralMatcher.getSimilarityPercentage(queryTokens, sourceTokens);

    result.exactSegments = findExactSegments(queryTokens, sourceTokens);
    return result;
}

std::vector<SegmentMatch> DetectionCore::findExactSegments(
    const std::vector<std::string>& queryTokens,
    const std::vector<std::string>& sourceTokens) const {

    std::vector<SegmentMatch> segments;
    if (queryTokens.size() < 3 || sourceTokens.empty()) {
        return segments;
    }

    int minLen = std::max(3, static_cast<int>(
        std::min(queryTokens.size(), sourceTokens.size()) / 5));
    minLen = std::min(minLen, static_cast<int>(sourceTokens.size()));

    for (int patLen = static_cast<int>(sourceTokens.size());
         patLen >= minLen; --patLen) {
        for (size_t s = 0; s + static_cast<size_t>(patLen) <= sourceTokens.size(); ++s) {
            std::vector<std::string> pattern(
                sourceTokens.begin() + static_cast<std::ptrdiff_t>(s),
                sourceTokens.begin() + static_cast<std::ptrdiff_t>(s + patLen));

            const auto hits = kmpSearchTokens(queryTokens, pattern);
            for (const int start : hits) {
                SegmentMatch seg;
                seg.tokenStart = start;
                seg.tokenEnd   = start + patLen;
                seg.weight     = static_cast<double>(patLen);
                seg.isExact    = true;
                segments.push_back(seg);
            }
        }
    }
    return segments;
}
