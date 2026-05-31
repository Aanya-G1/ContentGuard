#include "structuralSimilarity/Structural_Similarity.h"
#include "pipeline/token_bridge.hpp"
#include<iostream>
using namespace std;
StructuralSimilarity::StructuralSimilarity(double limit) : threshold(limit) {}
int StructuralSimilarity::LCS(const vector<string>& doc1, const vector<string>& doc2){
    if (doc1.empty() || doc2.empty()) return 0;
    int n = doc1.size(), m = doc2.size();
    vector<vector<int>> dp(n+1, vector<int>(m+1, 0));
    for(int i = 1; i <= n; i++) {
        for(int j = 1; j <= m; j++) {
            if(doc1[i-1] == doc2[j-1]) {
                dp[i][j] = 1 + dp[i-1][j-1];
            } else {
                dp[i][j] = max(dp[i-1][j], dp[i][j-1]);
            }
        }
    }
    return dp[n][m];
}
double StructuralSimilarity::getSimilarityPercentage(const vector<string>& doc1, const vector<string>& doc2){
    int lcs=LCS(doc1,doc2);
    return (2.0 * lcs) / (doc1.size() + doc2.size());
}
bool StructuralSimilarity::isMatch(double score){
    return(score>threshold);
}

void StructuralSimilarity::printMatch(int docId, double score) {
    cout << "Structural match with Doc ID: " << docId
         << " | Similarity: " << (score * 100.0) << "%" << endl;
}

vector<StructuralResult> StructuralPlagiarism(const vector<string>& currentTokens, vector<int> suspects) {
    vector<StructuralResult> finalReport;
    StructuralSimilarity matcher(0.45);
    for(int susId : suspects) {
        vector<string> dbTokens = getTokensBydocId(susId); 
        
        double score = matcher.getSimilarityPercentage(currentTokens, dbTokens);
        
        if (matcher.isMatch(score)) {
            string level = (score > 0.7) ? "CRITICAL" : "MODERATE";
            finalReport.push_back({susId, score * 100, level});
        }
    }
    return finalReport;
}