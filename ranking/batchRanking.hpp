#pragma once

#include <vector>
#include <map>
#include <queue>
#include <string>
#include "RawMatch.hpp"
#include "RankedResult.hpp"

class BatchRankingModule {
public:
    std::vector<RankedResult> processBatch(const std::vector<RawMatch>& rawMatches) const {
        std::map<std::string, std::vector<RawMatch>> grouped;
        for (const auto& match : rawMatches) {
            grouped[match.getSubmissionId()].push_back(match);
        }

        std::priority_queue<RankedResult> heap;
        for (auto& entry : grouped) {
            double total = 0.0;
            int count = 0;
            for (const auto& match : entry.second) {
                total += combinedScore(match);
                ++count;
            }
            double avg = (count == 0) ? 0.0 : total / count;
            heap.push(RankedResult(entry.first, avg, classify(avg), entry.second));
        }

        std::vector<RankedResult> ranked;
        int rank = 1;
        while (!heap.empty()) {
            RankedResult top = heap.top();
            heap.pop();
            top.setRank(rank++);
            ranked.push_back(top);
        }
        return ranked;
    }

private:
    static constexpr double EXACT_WEIGHT = 0.60;
    static constexpr double STRUCTURAL_WEIGHT = 0.40;

    double combinedScore(const RawMatch& match) const {
        return ((match.getExactScore() * EXACT_WEIGHT) +
                (match.getStructuralScore() * STRUCTURAL_WEIGHT)) * 100.0;
    }

    static std::string classify(double score) {
        if (score >= 70.0) return "HIGH";
        if (score >= 40.0) return "MEDIUM";
        if (score >= 10.0) return "LOW";
        return "NONE";
    }
};
