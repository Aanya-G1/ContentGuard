/**
 * ContentGuard — Step 4 Scoring & Ranking
 * GreedyScorer merge + std::priority_queue max-heap (BatchRankingModule)
 */

#include "ranking_module.hpp"

#include <algorithm>

PairScoreResult RankingModule::scorePair(
    int queryDocId,
    int sourceDocId,
    const DetectionResult& detection,
    const std::string& rawContent,
    const std::vector<std::string>& queryTokens) const {

    GreedyScorer scorer;
    const GreedyScoreResult greedy = scorer.merge(
        detection.exactSimilarity,
        detection.structuralSimilarity,
        detection.exactSegments,
        rawContent,
        queryTokens);

    int hlStart = 0;
    int hlEnd   = 0;
    std::string fragment;
    if (!greedy.highlights.empty()) {
        hlStart = greedy.highlights.front().start;
        hlEnd   = greedy.highlights.front().end;
        if (hlEnd > hlStart &&
            hlEnd <= static_cast<int>(rawContent.size())) {
            fragment = rawContent.substr(static_cast<size_t>(hlStart),
                                         static_cast<size_t>(hlEnd - hlStart));
        }
    }

    return PairScoreResult(
        greedy,
        RawMatch(
            "SUB-" + std::to_string(queryDocId),
            "SUB-" + std::to_string(sourceDocId),
            greedy.exactScore,
            greedy.structuralScore,
            fragment,
            hlStart,
            hlEnd));
}

std::vector<RankedResult> RankingModule::rankBatch(
    const std::vector<RawMatch>& rawMatches) const {

    BatchRankingModule ranker;
    return ranker.processBatch(rawMatches);
}

void RankingModule::applyRanksToScores(
    const std::vector<RankedResult>& ranked,
    std::unordered_map<int, int>& outRankByDocId,
    std::unordered_map<int, double>& outScoreByDocId,
    std::unordered_map<int, std::string>& outLabelByDocId) {

    for (const auto& r : ranked) {
        const std::string& id = r.getSubmissionId();
        if (id.size() < 5 || id.rfind("SUB-", 0) != 0) {
            continue;
        }
        const int docId = std::stoi(id.substr(4));
        outRankByDocId[docId] = r.getRank();
        outScoreByDocId[docId] =
            std::max(outScoreByDocId[docId], r.getAggregatedScore());
        outLabelByDocId[docId] = r.getPlagiarismLabel();
    }
}
