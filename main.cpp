#include <iostream>
using namespace std;
#include <vector>
#include "RawMatch.hpp"
#include "BatchRankingModule.hpp"
int main() {
    vector<RawMatch> matches;
    matches.push_back(
        RawMatch(
            "SUB001",
            "SRC101",
            0.90,
            0.75,
            "copied fragment",
            0,
            100
        )
    );
    matches.push_back(
        RawMatch(
            "SUB001",
            "SRC102",
            0.80,
            0.65,
            "another fragment",
            101,
            200
        )
    );
    BatchRankingModule rankingModule;
    auto results = rankingModule.processBatch(matches);
    for (const auto& result : results) {
        cout
            << "Rank: " << result.getRank()
            << " | Submission: " << result.getSubmissionId()
            << " | Score: " << result.getAggregatedScore()
            << "%"
            << endl;
    }
    return 0;
}
