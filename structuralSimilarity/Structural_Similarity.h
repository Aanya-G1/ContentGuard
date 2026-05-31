#ifndef STRUCTURAL_SIMILARITY_H
#define STRUCTURAL_SIMILARITY_H

#include <vector>
#include <string>
#include <algorithm>

struct StructuralResult {
    int docId;
    double percentage;
    std::string riskLevel;
};
class StructuralSimilarity {
private:
    double threshold;
    int LCS(const std::vector<std::string>& doc1, const std::vector<std::string>& doc2);
public:
    StructuralSimilarity(double limit);

    double getSimilarityPercentage(const std::vector<std::string>& doc1, const std::vector<std::string>& doc2);
    
    void printMatch(int docId, double score);

    bool isMatch(double score);
};

std::vector<StructuralResult> StructuralPlagiarism(const std::vector<std::string>& currentTokens,
                                                   std::vector<int> suspects);
#endif
