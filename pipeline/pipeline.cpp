/**
 * ContentGuard — Pipeline orchestrator (Steps 3–4)
 * Frontend → Preprocess → Fenced fingerprint → KMP/LCS → Greedy → Heap rank
 */

#include "pipeline/pipeline.hpp"
#include "pipeline/token_bridge.hpp"
#include "pipeline/detection_core.hpp"
#include "ranking/ranking_module.hpp"
#include "reporting/reporting.hpp"
#include "reporting/report_store.hpp"
#include "ranking/RawMatch.hpp"

#include "Fingerprinting/Fingerprint_module.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>

ContentGuardPipeline::ContentGuardPipeline(const std::string& connString,
                                           SubmissionManager& mgr,
                                           PreprocessorDB& ppdb)
    : connString_(connString), mgr_(mgr), ppdb_(ppdb),
      store_(new ReportStore(connString)) {
    registerTokenBridge(&ppdb_);
}

ContentGuardPipeline::~ContentGuardPipeline() {
    delete store_;
}

void ContentGuardPipeline::hydrateFromDatabase() {
    std::lock_guard<std::mutex> lock(mutex_);
    store_->loadAll(reports_);
}

void ContentGuardPipeline::persistReport(const ScanReport& report) {
    store_->save(report);
}

std::vector<SegmentMatch> ContentGuardPipeline::findExactSegments(
    const std::vector<std::string>& query,
    const std::vector<std::string>& source) const {
    DetectionCore core;
    return core.findExactSegments(query, source);
}

ScanReport ContentGuardPipeline::analyzeSubmission(int docId,
                                                   const std::string& rawContent) {
    ScanReport report;
    report.docId = docId;

    std::ostringstream batch;
    batch << "BATCH-" << std::hex << std::uppercase << docId;
    report.batchId = batch.str();

    // Milestone 1 handoff: tokenized document from PreprocessedDocs
    std::vector<std::string> queryTokens;
    try {
        queryTokens = ppdb_.getTokensByDocId(docId);
    } catch (...) {
        std::lock_guard<std::mutex> lock(mutex_);
        reports_[docId] = report;
        return report;
    }

    // Fenced: rolling-hash fingerprint filter (PostgreSQL)
    std::vector<int> suspects =
        fingerprintProcessing(docId, queryTokens, connString_);
    suspects.erase(std::remove(suspects.begin(), suspects.end(), docId),
                   suspects.end());

    DetectionCore detection;
    RankingModule ranking;
    std::vector<RawMatch> rawMatches;

    double bestScore = 0.0;
    std::string bestClass = "NONE";
    std::string bestLabel = "NONE";
    std::vector<HighlightRange> bestHighlights;

    for (int susId : suspects) {
        std::vector<std::string> sourceTokens;
        try {
            sourceTokens = ppdb_.getTokensByDocId(susId);
        } catch (...) {
            continue;
        }

        DetectionResult det = detection.compare(queryTokens, sourceTokens);
        PairScoreResult pair = ranking.scorePair(
            docId, susId, det, rawContent, queryTokens);

        MatchDetail md;
        md.sourceDocId     = susId;
        md.exactScore      = pair.greedy.exactScore;
        md.structuralScore = pair.greedy.structuralScore;
        md.combinedPercent = pair.greedy.finalPercent;
        md.classification  = pair.greedy.classification;
        md.highlights      = pair.greedy.highlights;
        report.matches.push_back(md);
        rawMatches.push_back(pair.rawMatch);

        if (pair.greedy.finalPercent > bestScore) {
            bestScore      = pair.greedy.finalPercent;
            bestClass      = pair.greedy.classification;
            bestLabel      = pair.greedy.label;
            bestHighlights = pair.greedy.highlights;
        }
    }

    if (suspects.empty()) {
        GreedyScorer scorer;
        auto empty = scorer.merge(0.0, 0.0, {}, rawContent, queryTokens);
        bestScore = empty.finalPercent;
        bestClass = empty.classification;
        bestLabel = empty.label;
    }

    report.score          = bestScore;
    report.classification = bestClass;
    report.label          = bestLabel;
    report.highlights     = bestHighlights;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        reports_[docId] = report;
    }

    recomputeBatchRanks();
    ScanReport finalReport = getReport(docId);
    persistReport(finalReport);
    return finalReport;
}

void ContentGuardPipeline::clearAllReports() {
    std::lock_guard<std::mutex> lock(mutex_);
    reports_.clear();
    store_->clearAll();
}

void ContentGuardPipeline::recomputeBatchRanks() {
    std::vector<RawMatch> allRaw;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& kv : reports_) {
            for (const auto& m : kv.second.matches) {
                int hlStart = m.highlights.empty() ? 0 : m.highlights.front().start;
                int hlEnd   = m.highlights.empty() ? 0 : m.highlights.front().end;
                allRaw.emplace_back(
                    "SUB-" + std::to_string(kv.first),
                    "SUB-" + std::to_string(m.sourceDocId),
                    m.exactScore,
                    m.structuralScore,
                    "",
                    hlStart,
                    hlEnd);
            }
            if (kv.second.matches.empty()) {
                allRaw.emplace_back(
                    "SUB-" + std::to_string(kv.first),
                    "NONE", 0.0, 0.0, "", 0, 0);
            }
        }
    }

    RankingModule ranking;
    auto ranked = ranking.rankBatch(allRaw);

    std::unordered_map<int, int>         ranks;
    std::unordered_map<int, double>      scores;
    std::unordered_map<int, std::string> labels;
    RankingModule::applyRanksToScores(ranked, ranks, scores, labels);

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& kv : reports_) {
        const int id = kv.first;
        if (ranks.count(id))  kv.second.rank  = ranks[id];
        if (scores.count(id)) kv.second.score = scores[id];
        if (labels.count(id)) kv.second.label = labels[id];
    }

    for (const auto& kv : reports_) {
        persistReport(kv.second);
    }
}

ScanReport ContentGuardPipeline::getReport(int docId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = reports_.find(docId);
    if (it != reports_.end()) return it->second;

    ScanReport loaded;
    if (store_->load(docId, loaded)) {
        reports_[docId] = loaded;
        return loaded;
    }
    return ScanReport{};
}

std::vector<ScanReport> ContentGuardPipeline::getAllReports() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ScanReport> out;
    out.reserve(reports_.size());
    for (const auto& kv : reports_) {
        out.push_back(kv.second);
    }
    return out;
}

nlohmann::json ContentGuardPipeline::reportToJson(const ScanReport& r) const {
    return ReportingModule::buildReportJson(r);
}

nlohmann::json ContentGuardPipeline::rankedListJson() const {
    return ReportingModule::buildRankedListJson(getAllReports());
}
