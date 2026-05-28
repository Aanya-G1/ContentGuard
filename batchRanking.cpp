#ifndef BATCHRANKINGMODULE_HPP
#define BATCHRANKINGMODULE_HPP
#include <vector>
#include <map>
#include <queue>
#include <set>
#include <algorithm>
#include "RawMatch.hpp"
#include "RankedResult.hpp"
class BatchRankingModule {
private:
    const double EXACT_WEIGHT = 0.60;
    const double STRUCTURAL_WEIGHT = 0.40;
    double combinedScore(const RawMatch& match) {
        return (
            (match.getExactScore() * EXACT_WEIGHT) +
            (match.getStructuralScore() * STRUCTURAL_WEIGHT)
        ) * 100;
    }
    string classify(double score) {
        if (score >= 70)
            return "HIGH";
        if (score >= 40)
            return "MEDIUM";
        if (score >= 10)
            return "LOW";
        return "NONE";
    }
public:
    vector<RankedResult> processBatch(
        const vector<RawMatch>& rawMatches
    ) {
        map<string, vector<RawMatch>> grouped;
        for (const auto& match : rawMatches) {
            grouped[match.getSubmissionId()].push_back(match);
        }
        priority_queue<RankedResult> heap;
        for (auto& entry : grouped) {
            double total = 0;
            int count = 0;
            for (const auto& match : entry.second) {
                total += combinedScore(match);
                count++;
            }
            double avg = (count == 0) ? 0 : total / count;
            RankedResult result(
                entry.first,
                avg,
                classify(avg),
                entry.second
            );
            heap.push(result);
        }
        vector<RankedResult> ranked;
        int rank = 1;
        while (!heap.empty()) {
            RankedResult top = heap.top();
            heap.pop();
            top.setRank(rank++);
            ranked.push_back(top);
        }
        return ranked;
    }
};
#endif
