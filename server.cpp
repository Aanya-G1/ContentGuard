#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "httplib.h"
#include "json.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

// ================= NORMALIZATION =================
std::string normalize(std::string s) {
    s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// ================= KMP =================
std::vector<int> buildLPS(const std::string &pattern) {
    int m = pattern.size();
    std::vector<int> lps(m, 0);

    int len = 0, i = 1;

    while (i < m) {
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        } else {
            if (len != 0) len = lps[len - 1];
            else lps[i++] = 0;
        }
    }
    return lps;
}

bool kmpSearch(const std::string &text, const std::string &pattern) {
    if (pattern.empty()) return false;

    int n = text.size(), m = pattern.size();
    std::vector<int> lps = buildLPS(pattern);

    int i = 0, j = 0;

    while (i < n) {
        if (text[i] == pattern[j]) {
            i++; j++;
        }

        if (j == m) return true;

        else if (i < n && text[i] != pattern[j]) {
            if (j != 0) j = lps[j - 1];
            else i++;
        }
    }
    return false;
}

// ================= TOKENIZATION =================
std::vector<std::string> tokenize(const std::string &text) {
    std::vector<std::string> tokens;
    std::string word;

    for (char c : text) {
        if (isspace(c)) {
            if (!word.empty()) {
                tokens.push_back(word);
                word.clear();
            }
        } else {
            word += tolower(c);
        }
    }

    if (!word.empty()) tokens.push_back(word);
    return tokens;
}

// ================= LCS =================
int lcs(const std::vector<std::string>& a,
        const std::vector<std::string>& b) {

    int n = a.size(), m = b.size();
    std::vector<std::vector<int>> dp(n+1, std::vector<int>(m+1, 0));

    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            if (a[i-1] == b[j-1])
                dp[i][j] = 1 + dp[i-1][j-1];
            else
                dp[i][j] = std::max(dp[i-1][j], dp[i][j-1]);
        }
    }

    return dp[n][m];
}

double structuralSimilarity(const std::vector<std::string>& a,
                            const std::vector<std::string>& b) {

    int l = lcs(a, b);
    int maxLen = std::max(a.size(), b.size());

    if (maxLen == 0) return 0;

    return (double)l / maxLen * 100.0;
}

// ================= FINAL SCORE =================
struct FinalScore {
    double exactWeight;
    double structural;
    double finalScore;
    std::string risk;
};

FinalScore computeScore(bool exact, double structural) {

    double exactWeight = exact ? 100.0 : 0.0;

    double finalScore = (0.4 * exactWeight) + (0.6 * structural);

    std::string risk;
    if (finalScore >= 70) risk = "CRITICAL";
    else if (finalScore >= 40) risk = "MODERATE";
    else risk = "LOW";

    return {exactWeight, structural, finalScore, risk};
}

// ================= CORS =================
void enable_cors(httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// ================= MAIN =================
int main() {

    httplib::Server svr;

    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        enable_cors(res);
    });

    // health check
    svr.Get("/api", [](const httplib::Request&, httplib::Response& res) {
        enable_cors(res);
        json j;
        j["status"] = "OK";
        res.set_content(j.dump(), "application/json");
    });

    // analysis API
    svr.Post("/check", [](const httplib::Request& req, httplib::Response& res) {

        enable_cors(res);

        try {
            auto body = json::parse(req.body);

            std::string text = body["text"];
            std::string pattern = body["pattern"];

            // ===== FIXED EXACT MATCH =====
            bool exactMatch =
                normalize(text) == normalize(pattern);

            // ===== STRUCTURAL =====
            auto t1 = tokenize(text);
            auto t2 = tokenize(pattern);

            double structural = structuralSimilarity(t1, t2);

            // ===== FINAL SCORE =====
            FinalScore fs = computeScore(exactMatch, structural);

            // ===== RESPONSE =====
            json result;

            result["exact_match"] = exactMatch;
            result["structural_similarity"] = structural;
            result["final_score"] = fs.finalScore;
            result["risk_level"] = fs.risk;

            res.set_content(result.dump(), "application/json");

        } catch (...) {
            json err;
            err["error"] = "Invalid request";
            res.status = 400;
            res.set_content(err.dump(), "application/json");
        }
    });

    std::cout << "Server running at http://localhost:5000\n";
    svr.listen("0.0.0.0", 5000);
}