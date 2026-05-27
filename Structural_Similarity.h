#ifndef Structural_Similarity
#define Structural_Similarity

#include <vector>
#include <string>
#include <algorithm>

struct StructuralResult {
    int docId;
    double percentage;
    string riskLevel; 
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

std::vector<StructuralResult > StructuralPlagiarism(const vector<string>& currentTokens,std::vector<int>suspects);
#endif