#ifndef RAWMATCH_HPP
#define RAWMATCH_HPP
#include <string>
#include <stdexcept>
#include <string>
#include <stdexcept>
#include <iostream>

using namespace std;
class RawMatch {
private:
    string submissionId;
    string matchedSourceId;
    double exactScore;
    double structuralScore;
    string matchedFragment;
    int matchStartIndex;
    int matchEndIndex;
    void validateScore(double value) {
        if (value < 0.0 || value > 1.0) {
            throw invalid_argument("Score must be between 0 and 1");
        }
    }
public:
    RawMatch(
        const string& submissionId,
        const string& matchedSourceId,
        double exactScore,
        double structuralScore,
        const string& matchedFragment,
        int matchStartIndex,
        int matchEndIndex
    ) : submissionId(submissionId),
        matchedSourceId(matchedSourceId),
        matchedFragment(matchedFragment),
        matchStartIndex(matchStartIndex),
        matchEndIndex(matchEndIndex) {
        validateScore(exactScore);
        validateScore(structuralScore);
        this->exactScore = exactScore;
        this->structuralScore = structuralScore;
    }
    string getSubmissionId() const {
        return submissionId;
    }
    string getMatchedSourceId() const {
        return matchedSourceId;
    }
    double getExactScore() const {
        return exactScore;
    }
    double getStructuralScore() const {
        return structuralScore;
    }
    string getMatchedFragment() const {
        return matchedFragment;
    }
    int getMatchStartIndex() const {
        return matchStartIndex;
    }
    int getMatchEndIndex() const {
        return matchEndIndex;
    }
};
#endif
