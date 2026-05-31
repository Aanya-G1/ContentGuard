/**
 * ContentGuard — Step 4 Greedy Scoring
 * Merges non-overlapping KMP segments with LCS structural signal.
 */

#include "scoring.hpp"

#include <algorithm>

GreedyScoreResult GreedyScorer::merge(
    double exactSimilarity,
    double structuralSimilarity,
    const std::vector<SegmentMatch>& exactSegments,
    const std::string& rawContent,
    const std::vector<std::string>& queryTokens) const {

    GreedyScoreResult out;
    out.exactScore      = std::clamp(exactSimilarity, 0.0, 1.0);
    out.structuralScore = std::clamp(structuralSimilarity, 0.0, 1.0);

    std::vector<SegmentMatch> sorted = exactSegments;
    std::sort(sorted.begin(), sorted.end(),
              [](const SegmentMatch& a, const SegmentMatch& b) {
                  const int lenA = a.tokenEnd - a.tokenStart;
                  const int lenB = b.tokenEnd - b.tokenStart;
                  if (lenA != lenB) return lenA > lenB;
                  return a.weight > b.weight;
              });

    std::vector<bool> used(queryTokens.size(), false);
    double greedyExact = 0.0;

    for (const auto& seg : sorted) {
        if (seg.tokenStart < 0 ||
            seg.tokenEnd > static_cast<int>(queryTokens.size())) {
            continue;
        }

        bool overlap = false;
        for (int t = seg.tokenStart; t < seg.tokenEnd; ++t) {
            if (used[static_cast<size_t>(t)]) {
                overlap = true;
                break;
            }
        }
        if (overlap) continue;

        for (int t = seg.tokenStart; t < seg.tokenEnd; ++t) {
            used[static_cast<size_t>(t)] = true;
        }

        const int len = seg.tokenEnd - seg.tokenStart;
        greedyExact += static_cast<double>(len);

        const ExactMatchSpan cs = tokenRangeToCharSpan(
            rawContent, queryTokens, seg.tokenStart, seg.tokenEnd);
        if (cs.end > cs.start) {
            out.highlights.push_back({cs.start, cs.end, "exact"});
        }
    }

    if (!queryTokens.empty()) {
        greedyExact /= static_cast<double>(queryTokens.size());
    }
    greedyExact = std::clamp(greedyExact, 0.0, 1.0);

    const double blendedExact = std::max(out.exactScore, greedyExact);
    out.finalPercent = (0.55 * blendedExact + 0.45 * out.structuralScore) * 100.0;
    out.classification = classify(blendedExact, out.structuralScore, out.finalPercent);
    out.label = riskLabel(out.finalPercent);

    if (out.structuralScore >= 0.35 && blendedExact < 0.25 &&
        !queryTokens.empty()) {
        const int mid = static_cast<int>(queryTokens.size()) / 4;
        const int end = std::min(
            static_cast<int>(queryTokens.size()),
            mid + static_cast<int>(queryTokens.size()) / 2);
        const ExactMatchSpan cs =
            tokenRangeToCharSpan(rawContent, queryTokens, mid, end);
        if (cs.end > cs.start) {
            out.highlights.push_back({cs.start, cs.end, "structural"});
        }
    }

    return out;
}

std::string GreedyScorer::classify(double exact,
                                   double structural,
                                   double finalPct) {
    if (exact >= 0.85) return "EXACT";
    if (structural >= 0.55 && exact < 0.30) return "REORDERED";
    if (finalPct >= 15.0 || exact >= 0.25 || structural >= 0.35) {
        return "PARTIAL";
    }
    return "NONE";
}

std::string GreedyScorer::riskLabel(double finalPct) {
    if (finalPct >= 70.0) return "HIGH";
    if (finalPct >= 40.0) return "MEDIUM";
    if (finalPct >= 10.0) return "LOW";
    return "NONE";
}
