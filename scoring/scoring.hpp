#pragma once

#include <string>
#include <vector>
#include "exact_match/Exact_Match.h"

struct HighlightRange {
    int start = 0;
    int end   = 0;
    std::string type;  // "exact" | "structural"
};

struct SegmentMatch {
    int tokenStart = 0;
    int tokenEnd   = 0;
    double weight  = 0.0;
    bool isExact   = true;
};

struct GreedyScoreResult {
    double exactScore       = 0.0;   // 0–1
    double structuralScore  = 0.0;   // 0–1
    double finalPercent     = 0.0;   // 0–100
    std::string classification;    // EXACT | PARTIAL | REORDERED | NONE
    std::string label;             // HIGH | MEDIUM | LOW | NONE
    std::vector<HighlightRange> highlights;
};

/**
 * Greedy merge of exact (KMP) token segments and structural (LCS) signal.
 * Picks non-overlapping segments by descending weight, then classifies.
 */
class GreedyScorer {
public:
    GreedyScoreResult merge(double exactSimilarity,
                            double structuralSimilarity,
                            const std::vector<SegmentMatch>& exactSegments,
                            const std::string& rawContent,
                            const std::vector<std::string>& queryTokens) const;

private:
    static std::string classify(double exact, double structural, double finalPct);
    static std::string riskLabel(double finalPct);
};
