#include "report_store.hpp"
#include "reporting.hpp"
#include "json.hpp"
#include <pqxx/pqxx>
#include <unordered_map>

ReportStore::ReportStore(const std::string& connString)
    : connString_(connString) {}

void ReportStore::save(const ScanReport& report) {
    pqxx::connection conn(connString_);
    pqxx::work txn(conn);
    txn.exec(
        "CREATE TABLE IF NOT EXISTS scan_results ("
        "  doc_id INT PRIMARY KEY,"
        "  score DOUBLE PRECISION NOT NULL DEFAULT 0,"
        "  label VARCHAR(16),"
        "  classification VARCHAR(16),"
        "  rank_position INT DEFAULT 0,"
        "  batch_id VARCHAR(64),"
        "  report_json TEXT NOT NULL,"
        "  updated_at TIMESTAMP NOT NULL DEFAULT NOW()"
        ")");
    const std::string json = ReportingModule::buildReportJson(report).dump();
    txn.exec(
        "INSERT INTO scan_results"
        "  (doc_id, score, label, classification, rank_position, batch_id, report_json)"
        " VALUES ($1,$2,$3,$4,$5,$6,$7)"
        " ON CONFLICT (doc_id) DO UPDATE SET"
        "  score=$2, label=$3, classification=$4, rank_position=$5,"
        "  batch_id=$6, report_json=$7, updated_at=NOW()",
        pqxx::params{
            report.docId,
            report.score,
            report.label,
            report.classification,
            report.rank,
            report.batchId,
            json});
    txn.commit();
}

bool ReportStore::load(int docId, ScanReport& out) const {
    pqxx::connection conn(connString_);
    pqxx::read_transaction txn(conn);
    auto rows = txn.exec(
        "SELECT doc_id, score, label, classification, rank_position, batch_id, report_json"
        " FROM scan_results WHERE doc_id=$1",
        pqxx::params{docId});
    if (rows.empty()) return false;

    const auto& row = rows[0];
    out.docId          = row[0].as<int>();
    out.score          = row[1].as<double>();
    out.label          = row[2].c_str();
    out.classification = row[3].c_str();
    out.rank           = row[4].as<int>();
    out.batchId        = row[5].c_str();

    auto j = nlohmann::json::parse(row[6].c_str());
    out.highlights.clear();
    if (j.contains("highlights")) {
        for (const auto& h : j["highlights"]) {
            HighlightRange hr;
            hr.start = h.value("start", 0);
            hr.end   = h.value("end", 0);
            hr.type  = h.value("type", "exact");
            out.highlights.push_back(hr);
        }
    }
    out.matches.clear();
    if (j.contains("matches")) {
        for (const auto& m : j["matches"]) {
            MatchDetail md;
            md.sourceDocId       = m.value("source_doc_id", 0);
            md.exactScore        = m.value("exact_score", 0.0);
            md.structuralScore   = m.value("structural_score", 0.0);
            md.combinedPercent   = m.value("combined_percent", 0.0);
            md.classification    = m.value("classification", "NONE");
            if (m.contains("highlights")) {
                for (const auto& h : m["highlights"]) {
                    md.highlights.push_back({
                        h.value("start", 0),
                        h.value("end", 0),
                        h.value("type", "exact")
                    });
                }
            }
            out.matches.push_back(md);
        }
    }
    return true;
}

void ReportStore::remove(int docId) {
    pqxx::connection conn(connString_);
    pqxx::work txn(conn);
    txn.exec(
        "DELETE FROM scan_results WHERE doc_id=$1",
        pqxx::params{docId});
    txn.commit();
}

void ReportStore::clearAll() {
    pqxx::connection conn(connString_);
    pqxx::work txn(conn);
    txn.exec("DELETE FROM scan_results");
    txn.commit();
}

void ReportStore::loadAll(std::unordered_map<int, ScanReport>& out) const {
    pqxx::connection conn(connString_);
    pqxx::read_transaction txn(conn);
    auto rows = txn.exec("SELECT doc_id FROM scan_results ORDER BY doc_id");
    for (const auto& row : rows) {
        ScanReport r;
        if (load(row[0].as<int>(), r)) {
            out[r.docId] = r;
        }
    }
}
