#pragma once

/**
 * Step 4 — Scoring & Ranking (unfenced)
 *
 *   - GreedyScorer: merge KMP segments + LCS signal → classification
 *   - BatchRankingModule: max-heap (priority_queue) for batch severity order
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include "pipeline/detection_core.hpp"
#include "scoring/scoring.hpp"
#include "RawMatch.hpp"
#include "RankedResult.hpp"
#include "batchRanking.hpp"

struct PairScoreResult {
    GreedyScoreResult greedy;
    RawMatch rawMatch;

    PairScoreResult(GreedyScoreResult g, RawMatch m)
        : greedy(std::move(g)), rawMatch(std::move(m)) {}
};

class RankingModule {
public:
    /**
     * Greedy merge for one query-vs-source pair; builds RawMatch for heap input.
     */
    PairScoreResult scorePair(int queryDocId,
                              int sourceDocId,
                              const DetectionResult& detection,
                              const std::string& rawContent,
                              const std::vector<std::string>& queryTokens) const;

    /**
     * Rank all submissions in batch using std::priority_queue (max-heap).
     */
    std::vector<RankedResult> rankBatch(
        const std::vector<RawMatch>& rawMatches) const;

    /** Apply heap ranks back onto per-doc aggregated scores. */
    static void applyRanksToScores(
        const std::vector<RankedResult>& ranked,
        std::unordered_map<int, int>& outRankByDocId,
        std::unordered_map<int, double>& outScoreByDocId,
        std::unordered_map<int, std::string>& outLabelByDocId);
};
