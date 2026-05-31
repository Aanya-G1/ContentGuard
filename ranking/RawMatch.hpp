#pragma once

#include <string>
#include <stdexcept>

class RawMatch {
public:
    RawMatch(const std::string& submissionId,
             const std::string& matchedSourceId,
             double exactScore,
             double structuralScore,
             const std::string& matchedFragment,
             int matchStartIndex,
             int matchEndIndex)
        : submissionId_(submissionId),
          matchedSourceId_(matchedSourceId),
          matchedFragment_(matchedFragment),
          matchStartIndex_(matchStartIndex),
          matchEndIndex_(matchEndIndex) {
        validateScore(exactScore);
        validateScore(structuralScore);
        exactScore_ = exactScore;
        structuralScore_ = structuralScore;
    }

    std::string getSubmissionId() const { return submissionId_; }
    std::string getMatchedSourceId() const { return matchedSourceId_; }
    double getExactScore() const { return exactScore_; }
    double getStructuralScore() const { return structuralScore_; }
    std::string getMatchedFragment() const { return matchedFragment_; }
    int getMatchStartIndex() const { return matchStartIndex_; }
    int getMatchEndIndex() const { return matchEndIndex_; }

private:
    std::string submissionId_;
    std::string matchedSourceId_;
    double exactScore_ = 0.0;
    double structuralScore_ = 0.0;
    std::string matchedFragment_;
    int matchStartIndex_ = 0;
    int matchEndIndex_ = 0;

    static void validateScore(double value) {
        if (value < 0.0 || value > 1.0)
            throw std::invalid_argument("Score must be between 0 and 1");
    }
};
