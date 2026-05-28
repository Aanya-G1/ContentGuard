#ifndef RANKEDRESULT_HPP
#define RANKEDRESULT_HPP
#include <string>
#include <vector>
#include "RawMatch.hpp"
class RankedResult {
private:
    string submissionId;
    double aggregatedScore;
    string plagiarismLabel;
    int rank;
    vector<RawMatch> topMatches;
public:
    RankedResult(
        const string& submissionId,
        double aggregatedScore,
        const string& plagiarismLabel,
        const vector<RawMatch>& topMatches
    ) : submissionId(submissionId),
        aggregatedScore(aggregatedScore),
        plagiarismLabel(plagiarismLabel),
        topMatches(topMatches),
        rank(0) {}
    bool operator<(const RankedResult& other) const {
        return aggregatedScore < other.aggregatedScore;
    }
    void setRank(int r) {
        rank = r;
    }
    int getRank() const {
        return rank;
    }
    double getAggregatedScore() const {
        return aggregatedScore;
    }
    string getSubmissionId() const {
        return submissionId;
    }
    string getPlagiarismLabel() const {
        return plagiarismLabel;
    }
};
#endif
