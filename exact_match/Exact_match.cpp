#include "exact_match/Exact_Match.h"

#include <algorithm>
#include <cctype>

void computeLPS(const std::string& pattern, std::vector<int>& lps) {
    int m = static_cast<int>(pattern.size());
    lps.assign(m, 0);
    int len = 0;
    int i = 1;
    while (i < m) {
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        } else if (len != 0) {
            len = lps[len - 1];
        } else {
            lps[i++] = 0;
        }
    }
}

std::vector<int> kmpSearchAll(const std::string& text, const std::string& pattern) {
    std::vector<int> hits;
    if (pattern.empty() || text.size() < pattern.size()) return hits;

    std::vector<int> lps;
    computeLPS(pattern, lps);

    int n = static_cast<int>(text.size());
    int m = static_cast<int>(pattern.size());
    int i = 0;
    int j = 0;

    while (i < n) {
        if (text[i] == pattern[j]) {
            ++i;
            ++j;
        }
        if (j == m) {
            hits.push_back(i - j);
            j = lps[j - 1];
        } else if (i < n && text[i] != pattern[j]) {
            if (j != 0) j = lps[j - 1];
            else ++i;
        }
    }
    return hits;
}

static void computeTokenLPS(const std::vector<std::string>& pattern,
                            std::vector<int>& lps) {
    int m = static_cast<int>(pattern.size());
    lps.assign(m, 0);
    int len = 0;
    int i = 1;
    while (i < m) {
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        } else if (len != 0) {
            len = lps[len - 1];
        } else {
            lps[i++] = 0;
        }
    }
}

std::vector<int> kmpSearchTokens(const std::vector<std::string>& text,
                                 const std::vector<std::string>& pattern) {
    std::vector<int> hits;
    if (pattern.empty() || text.size() < pattern.size()) return hits;

    std::vector<int> lps;
    computeTokenLPS(pattern, lps);

    int n = static_cast<int>(text.size());
    int m = static_cast<int>(pattern.size());
    int i = 0;
    int j = 0;

    while (i < n) {
        if (text[i] == pattern[j]) {
            ++i;
            ++j;
        }
        if (j == m) {
            hits.push_back(i - j);
            j = lps[j - 1];
        } else if (i < n && text[i] != pattern[j]) {
            if (j != 0) j = lps[j - 1];
            else ++i;
        }
    }
    return hits;
}

double computeExactTokenSimilarity(const std::vector<std::string>& queryTokens,
                                   const std::vector<std::string>& sourceTokens) {
    if (queryTokens.empty() || sourceTokens.empty()) return 0.0;

    const auto& shorter = queryTokens.size() <= sourceTokens.size() ? queryTokens : sourceTokens;
    const auto& longer  = queryTokens.size() <= sourceTokens.size() ? sourceTokens : queryTokens;

    int minPat = std::max(3, static_cast<int>(shorter.size()) / 4);
    minPat = std::min(minPat, static_cast<int>(shorter.size()));
    if (minPat < 1) return 0.0;

    std::vector<bool> covered(longer.size(), false);
    int matchedTokens = 0;

    for (int patLen = static_cast<int>(shorter.size()); patLen >= minPat; --patLen) {
        for (size_t s = 0; s + patLen <= shorter.size(); ++s) {
            std::vector<std::string> pattern(shorter.begin() + s,
                                             shorter.begin() + s + patLen);
            auto hits = kmpSearchTokens(longer, pattern);
            for (int start : hits) {
                for (int k = 0; k < patLen; ++k) {
                    if (!covered[start + k]) {
                        covered[start + k] = true;
                        ++matchedTokens;
                    }
                }
            }
        }
        if (matchedTokens > static_cast<int>(longer.size()) / 2) break;
    }

    return static_cast<double>(matchedTokens) /
           static_cast<double>(std::max(queryTokens.size(), sourceTokens.size()));
}

ExactMatchSpan tokenRangeToCharSpan(const std::string& rawContent,
                                    const std::vector<std::string>& tokens,
                                    int tokenStart,
                                    int tokenEnd) {
    ExactMatchSpan span;
    if (tokens.empty() || tokenStart < 0 || tokenEnd <= tokenStart) return span;

    tokenEnd = std::min(tokenEnd, static_cast<int>(tokens.size()));
    tokenStart = std::max(0, tokenStart);

    size_t pos = 0;
    int idx = 0;
    int charStart = -1;
    int charEnd = -1;

    while (pos < rawContent.size() && idx <= tokenEnd) {
        while (pos < rawContent.size() && std::isspace(static_cast<unsigned char>(rawContent[pos])))
            ++pos;
        if (pos >= rawContent.size()) break;

        size_t wordStart = pos;
        while (pos < rawContent.size() &&
               !std::isspace(static_cast<unsigned char>(rawContent[pos])))
            ++pos;

        if (idx == tokenStart) charStart = static_cast<int>(wordStart);
        if (idx == tokenEnd - 1) {
            charEnd = static_cast<int>(pos);
            break;
        }
        ++idx;
    }

    if (charStart >= 0 && charEnd > charStart) {
        span.start = charStart;
        span.end = charEnd;
    }
    return span;
}
