#pragma once

#include <string>
#include <vector>
#include "RawMatch.hpp"

class RankedResult {
public:
    RankedResult(const std::string& submissionId,
                 double aggregatedScore,
                 const std::string& plagiarismLabel,
                 const std::vector<RawMatch>& topMatches)
        : submissionId_(submissionId),
          aggregatedScore_(aggregatedScore),
          plagiarismLabel_(plagiarismLabel),
          rank_(0),
          topMatches_(topMatches) {}

    bool operator<(const RankedResult& other) const {
        return aggregatedScore_ < other.aggregatedScore_;
    }

    void setRank(int r) { rank_ = r; }
    int getRank() const { return rank_; }
    double getAggregatedScore() const { return aggregatedScore_; }
    std::string getSubmissionId() const { return submissionId_; }
    std::string getPlagiarismLabel() const { return plagiarismLabel_; }
    const std::vector<RawMatch>& getTopMatches() const { return topMatches_; }

private:
    std::string submissionId_;
    double aggregatedScore_ = 0.0;
    std::string plagiarismLabel_;
    int rank_ = 0;
    std::vector<RawMatch> topMatches_;
};
