#pragma once

/**
 * ContentGuard pipeline orchestrator (Steps 3–4).
 *
 * Stages inside analyzeSubmission():
 *   1. Load tokens (PreprocessorDB / Milestone 1)
 *   2. Fingerprint filter (FingerPrint.cpp — fenced)
 *   3. DetectionCore — KMP + LCS
 *   4. RankingModule — greedy merge + heap batch rank
 */

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "json.hpp"
#include "scoring/scoring.hpp"
#include "preprocessor/preprocessor_db.hpp"
#include "submission_manager.hpp"

class ReportStore;

struct MatchDetail {
    int sourceDocId = 0;
    double exactScore = 0.0;
    double structuralScore = 0.0;
    double combinedPercent = 0.0;
    std::string classification;
    std::vector<HighlightRange> highlights;
};

struct ScanReport {
    int docId = 0;
    double score = 0.0;
    std::string label;
    std::string classification;
    std::string batchId;
    std::vector<MatchDetail> matches;
    std::vector<HighlightRange> highlights;
    int rank = 0;
};

class ContentGuardPipeline {
public:
    ContentGuardPipeline(const std::string& connString,
                         SubmissionManager& mgr,
                         PreprocessorDB& ppdb);
    ~ContentGuardPipeline();

    /** Load persisted reports from PostgreSQL into memory cache. */
    void hydrateFromDatabase();

    ScanReport analyzeSubmission(int docId, const std::string& rawContent);

    void recomputeBatchRanks();
    void clearAllReports();

    ScanReport getReport(int docId);
    std::vector<ScanReport> getAllReports() const;

    nlohmann::json reportToJson(const ScanReport& r) const;

    /** All cached reports sorted by heap rank (Step 4 output). */
    nlohmann::json rankedListJson() const;

private:
    std::string connString_;
    SubmissionManager& mgr_;
    PreprocessorDB& ppdb_;
    ReportStore* store_;

    mutable std::mutex mutex_;
    std::unordered_map<int, ScanReport> reports_;

    void persistReport(const ScanReport& report);

    std::vector<SegmentMatch> findExactSegments(
        const std::vector<std::string>& query,
        const std::vector<std::string>& source) const;
};
