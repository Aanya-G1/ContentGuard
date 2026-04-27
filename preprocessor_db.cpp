#include "preprocessor_db.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>

// ─────────────────────────────────────────────────────────────
// Constructor — connects to Neon PostgreSQL
// ─────────────────────────────────────────────────────────────
PreprocessorDB::PreprocessorDB(const std::string& connString)
    : conn_(connString) {}

// ─────────────────────────────────────────────────────────────
// processSubmission
// Reads one submission, runs pipeline, saves result
// ─────────────────────────────────────────────────────────────
int PreprocessorDB::processSubmission(int doc_id) {
    // Step 1: Read submission from Submissions table
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT content, fileName, fileType"
        " FROM Submissions WHERE doc_id=$1",
        doc_id);
    txn.commit();

    if (r.empty())
        throw std::runtime_error(
            "Submission not found: doc_id=" + std::to_string(doc_id));

    std::string content  = r[0][0].c_str();
    std::string fileName = r[0][1].c_str();
    std::string fileType = r[0][2].c_str();

    // Step 2: Run the preprocessing pipeline
    FileType ft = (fileType == "CODE") ? FileType::CODE : FileType::TEXT;
    Preprocessor pp(ft, /*removeStopWords=*/true, /*normalizeIds=*/true);
    ProcessedDoc doc = pp.process(doc_id, content);

    std::string detectedStr = (doc.detectedType == FileType::CODE) ? "CODE" : "TEXT";

    // Step 3: Save result to PreprocessedDocs
    // preprocessId is SERIAL — auto-assigned by PostgreSQL
    pqxx::work txn2(conn_);
    auto res = txn2.exec_params(
        "INSERT INTO PreprocessedDocs"
        "  (doc_id, normalizedContent, tokenCount, charCount, detectedType)"
        "  VALUES($1, $2, $3, $4, $5)"
        "  RETURNING preprocessId",
        doc_id,
        doc.normalizedContent,
        doc.tokenCount,
        doc.charCount,
        detectedStr);

    // Step 4: Update submission status → DONE
    txn2.exec_params(
        "UPDATE Submissions SET status='DONE' WHERE doc_id=$1",
        doc_id);

    txn2.commit();

    int preprocessId = res[0][0].as<int>();

    std::cout << "  [Module 2] doc_id=" << doc_id
              << " -> preprocessId=" << preprocessId
              << " | tokens=" << doc.tokenCount
              << " | type=" << detectedStr << "\n";

    return preprocessId;
}

// ─────────────────────────────────────────────────────────────
// processAllPending
// Processes all submissions with status='PROCESSING'
// ─────────────────────────────────────────────────────────────
int PreprocessorDB::processAllPending() {
    // Find all submissions waiting to be processed
    pqxx::work txn(conn_);
    auto rows = txn.exec(
        "SELECT doc_id FROM Submissions"
        " WHERE status='PROCESSING' ORDER BY doc_id");
    txn.commit();

    if (rows.empty()) {
        std::cout << "  [Module 2] No submissions in PROCESSING queue.\n";
        return 0;
    }

    std::cout << "  [Module 2] Found " << rows.size()
              << " submission(s) to process.\n";

    int count = 0;
    for (const auto& row : rows) {
        int id = row[0].as<int>();
        try {
            processSubmission(id);
            ++count;
        } catch (const std::exception& e) {
            std::cerr << "  [Module 2] ERROR on doc_id="
                      << id << ": " << e.what() << "\n";
            // Mark as ERROR so it doesn't block the queue
            try {
                pqxx::work fix(conn_);
                fix.exec_params(
                    "UPDATE Submissions SET status='ERROR' WHERE doc_id=$1", id);
                fix.commit();
            } catch (...) {}
        }
    }
    return count;
}

// ─────────────────────────────────────────────────────────────
// getResult — fetch by preprocessId
// ─────────────────────────────────────────────────────────────
ProcessedDoc PreprocessorDB::getResult(int preprocessId) {
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT preprocessId, doc_id, normalizedContent,"
        "       tokenCount, charCount, detectedType"
        " FROM PreprocessedDocs WHERE preprocessId=$1",
        preprocessId);
    txn.commit();

    if (r.empty())
        throw std::runtime_error(
            "No result found: preprocessId=" + std::to_string(preprocessId));

    ProcessedDoc doc;
    doc.preprocessId      = r[0][0].as<int>();
    doc.doc_id            = r[0][1].as<int>();
    doc.normalizedContent = r[0][2].c_str();
    doc.tokenCount        = r[0][3].as<int>();
    doc.charCount         = r[0][4].as<int>();
    doc.detectedType      = (std::string(r[0][5].c_str()) == "CODE")
                            ? FileType::CODE : FileType::TEXT;
    return doc;
}

// ─────────────────────────────────────────────────────────────
// getResultByDocId — fetch by doc_id
// ─────────────────────────────────────────────────────────────
ProcessedDoc PreprocessorDB::getResultByDocId(int doc_id) {
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT preprocessId, doc_id, normalizedContent,"
        "       tokenCount, charCount, detectedType"
        " FROM PreprocessedDocs WHERE doc_id=$1",
        doc_id);
    txn.commit();

    if (r.empty())
        throw std::runtime_error(
            "No result found: doc_id=" + std::to_string(doc_id));

    ProcessedDoc doc;
    doc.preprocessId      = r[0][0].as<int>();
    doc.doc_id            = r[0][1].as<int>();
    doc.normalizedContent = r[0][2].c_str();
    doc.tokenCount        = r[0][3].as<int>();
    doc.charCount         = r[0][4].as<int>();
    doc.detectedType      = (std::string(r[0][5].c_str()) == "CODE")
                            ? FileType::CODE : FileType::TEXT;
    return doc;
}

// ─────────────────────────────────────────────────────────────
// getAllResults — fetch all preprocessed documents
// Used by Module 3 to get every document for fingerprinting
// ─────────────────────────────────────────────────────────────
std::vector<ProcessedDoc> PreprocessorDB::getAllResults() {
    pqxx::work txn(conn_);
    auto r = txn.exec(
        "SELECT preprocessId, doc_id, normalizedContent,"
        "       tokenCount, charCount, detectedType"
        " FROM PreprocessedDocs ORDER BY preprocessId");
    txn.commit();

    std::vector<ProcessedDoc> results;
    for (const auto& row : r) {
        ProcessedDoc doc;
        doc.preprocessId      = row[0].as<int>();
        doc.doc_id            = row[1].as<int>();
        doc.normalizedContent = row[2].c_str();
        doc.tokenCount        = row[3].as<int>();
        doc.charCount         = row[4].as<int>();
        doc.detectedType      = (std::string(row[5].c_str()) == "CODE")
                                ? FileType::CODE : FileType::TEXT;
        results.push_back(doc);
    }
    return results;
}

// ─────────────────────────────────────────────────────────────
// getTokensByDocId — PRIMARY HANDOFF FUNCTION FOR MODULE 3
//
// Given a doc_id, reads normalizedContent from PreprocessedDocs
// and splits it into individual tokens.
//
// normalizedContent is space-separated so splitting on spaces
// gives back the exact token list.
//
// Example:
//   normalizedContent = "int var 0 var 1 0 return 0"
//   returns = ["int","var","0","var","1","0","return","0"]
// ─────────────────────────────────────────────────────────────
std::vector<std::string> PreprocessorDB::getTokensByDocId(int doc_id) {
    // Fetch normalizedContent for this doc_id
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT normalizedContent"
        " FROM PreprocessedDocs WHERE doc_id=$1",
        doc_id);
    txn.commit();

    if (r.empty())
        throw std::runtime_error(
            "No preprocessed result for doc_id=" + std::to_string(doc_id)
            + ". Make sure Module 2 has processed this submission first.");

    // Split normalizedContent string into tokens
    std::string normalizedContent = r[0][0].c_str();
    std::vector<std::string> tokens;
    std::istringstream ss(normalizedContent);
    std::string token;
    while (ss >> token)
        tokens.push_back(token);

    return tokens;
}

// ─────────────────────────────────────────────────────────────
// isProcessed — check if a doc_id is already in PreprocessedDocs
// ─────────────────────────────────────────────────────────────
bool PreprocessorDB::isProcessed(int doc_id) {
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT 1 FROM PreprocessedDocs WHERE doc_id=$1", doc_id);
    txn.commit();
    return !r.empty();
}

// ─────────────────────────────────────────────────────────────
// deleteResult — remove a preprocessed result
// Resets status to PENDING so it can be re-processed
// ─────────────────────────────────────────────────────────────
void PreprocessorDB::deleteResult(int doc_id) {
    pqxx::work txn(conn_);
    txn.exec_params(
        "DELETE FROM PreprocessedDocs WHERE doc_id=$1", doc_id);
    txn.exec_params(
        "UPDATE Submissions SET status='PENDING' WHERE doc_id=$1", doc_id);
    txn.commit();
}
