#ifndef EXACT_MATCH_H
#define EXACT_MATCH_H

#include <string>
#include <vector>

/** Character span in the original submission text (for highlighting). */
struct ExactMatchSpan {
    int start = 0;
    int end   = 0;
};

/** Build LPS table for Knuth-Morris-Pratt (O(m)). */
void computeLPS(const std::string& pattern, std::vector<int>& lps);

/**
 * KMP search — returns start indices of every occurrence of pattern in text.
 * Time complexity: O(n + m).
 */
std::vector<int> kmpSearchAll(const std::string& text, const std::string& pattern);

/**
 * Token-level KMP — finds all start token indices where pattern occurs in text.
 */
std::vector<int> kmpSearchTokens(const std::vector<std::string>& text,
                                 const std::vector<std::string>& pattern);

/**
 * Exact similarity in [0, 1] using token-level KMP coverage vs the longer document.
 */
double computeExactTokenSimilarity(const std::vector<std::string>& queryTokens,
                                   const std::vector<std::string>& sourceTokens);

/**
 * Map token index range to character offsets in rawContent (whitespace-separated tokens).
 */
ExactMatchSpan tokenRangeToCharSpan(const std::string& rawContent,
                                    const std::vector<std::string>& tokens,
                                    int tokenStart,
                                    int tokenEnd);

#endif
