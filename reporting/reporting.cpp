/**
 * ContentGuard — Step 5 Reporting (JSON for frontend)
 * Similarity scores, classification, highlight ranges, ranked list.
 */

#include "reporting.hpp"

nlohmann::json ReportingModule::buildReportJson(const ScanReport& r) {
    nlohmann::json j;
    j["doc_id"]         = r.docId;
    j["score"]          = r.score;
    j["label"]          = r.label;
    j["classification"] = r.classification;
    j["batchId"]        = r.batchId;
    j["rank"]           = r.rank;

    j["highlights"] = nlohmann::json::array();
    for (const auto& h : r.highlights) {
        j["highlights"].push_back({
            {"start", h.start},
            {"end",   h.end},
            {"type",  h.type}
        });
    }

    j["matches"] = nlohmann::json::array();
    for (const auto& m : r.matches) {
        nlohmann::json mj = {
            {"source_doc_id",    m.sourceDocId},
            {"exact_score",      m.exactScore},
            {"structural_score", m.structuralScore},
            {"combined_percent", m.combinedPercent},
            {"classification",   m.classification}
        };
        mj["highlights"] = nlohmann::json::array();
        for (const auto& h : m.highlights) {
            mj["highlights"].push_back({
                {"start", h.start},
                {"end",   h.end},
                {"type",  h.type}
            });
        }
        j["matches"].push_back(mj);
    }
    return j;
}

nlohmann::json ReportingModule::buildSubmitResponse(
    const ScanReport& report,
    const nlohmann::json& metadata) {

    nlohmann::json response = metadata;
    response["success"]        = true;
    response["score"]          = report.score;
    response["label"]          = report.label;
    response["classification"] = report.classification;
    response["rank"]           = report.rank;
    response["batchId"]        = report.batchId;
    response["report"]         = buildReportJson(report);
    return response;
}

nlohmann::json ReportingModule::buildRankedListJson(
    const std::vector<ScanReport>& reports) {

    nlohmann::json list = nlohmann::json::array();
    for (const auto& r : reports) {
        list.push_back({
            {"doc_id",         r.docId},
            {"rank",           r.rank},
            {"score",          r.score},
            {"label",          r.label},
            {"classification", r.classification},
            {"batchId",        r.batchId}
        });
    }
    return list;
}
