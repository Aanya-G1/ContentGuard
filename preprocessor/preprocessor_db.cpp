#include "preprocessor_db.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>

PreprocessorDB::PreprocessorDB(const std::string& connString)
    : conn_(connString) {}

int PreprocessorDB::processSubmission(int doc_id) {
    pqxx::work txn(conn_);
    auto r = txn.exec(
        "SELECT content, fileName, fileType"
        " FROM Submissions WHERE doc_id=$1",
        pqxx::params{doc_id});
    txn.commit();

    if (r.empty())
        throw std::runtime_error(
            "Submission not found: doc_id=" + std::to_string(doc_id));

    std::string content  = r[0][0].c_str();
    std::string fileName = r[0][1].c_str();
    std::string fileType = r[0][2].c_str();

    FileType ft = (fileType == "CODE") ? FileType::CODE : FileType::TEXT;
    Preprocessor pp(ft, /*removeStopWords=*/true, /*normalizeIds=*/true);
    ProcessedDoc doc = pp.process(doc_id, content);

    std::string detectedStr = (doc.detectedType == FileType::CODE) ? "CODE" : "TEXT";

    pqxx::work txn2(conn_);
    auto res = txn2.exec(
        "INSERT INTO PreprocessedDocs"
        "  (doc_id, normalizedContent, tokenCount, charCount, detectedType)"
        "  VALUES($1, $2, $3, $4, $5)"
        "  RETURNING preprocessId",
        pqxx::params{
            doc_id,
            doc.normalizedContent,
            doc.tokenCount,
            doc.charCount,
            detectedStr});

    txn2.exec(
        "UPDATE Submissions SET status='DONE' WHERE doc_id=$1",
        pqxx::params{doc_id});

    txn2.commit();

    int preprocessId = res[0][0].as<int>();

    std::cout << "  [Module 2] doc_id=" << doc_id
              << " -> preprocessId=" << preprocessId
              << " | tokens=" << doc.tokenCount
              << " | type=" << detectedStr << "\n";

    return preprocessId;
}

int PreprocessorDB::processAllPending() {
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
            try {
                pqxx::work fix(conn_);
                fix.exec(
                    "UPDATE Submissions SET status='ERROR' WHERE doc_id=$1",
                    pqxx::params{id});
                fix.commit();
            } catch (...) {}
        }
    }
    return count;
}

ProcessedDoc PreprocessorDB::getResult(int preprocessId) {
    pqxx::work txn(conn_);
    auto r = txn.exec(
        "SELECT preprocessId, doc_id, normalizedContent,"
        "       tokenCount, charCount, detectedType"
        " FROM PreprocessedDocs WHERE preprocessId=$1",
        pqxx::params{preprocessId});
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

ProcessedDoc PreprocessorDB::getResultByDocId(int doc_id) {
    pqxx::work txn(conn_);
    auto r = txn.exec(
        "SELECT preprocessId, doc_id, normalizedContent,"
        "       tokenCount, charCount, detectedType"
        " FROM PreprocessedDocs WHERE doc_id=$1",
        pqxx::params{doc_id});
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

std::vector<std::string> PreprocessorDB::getTokensByDocId(int doc_id) {
    pqxx::work txn(conn_);
    auto r = txn.exec(
        "SELECT normalizedContent"
        " FROM PreprocessedDocs WHERE doc_id=$1",
        pqxx::params{doc_id});
    txn.commit();

    if (r.empty())
        throw std::runtime_error(
            "No preprocessed result for doc_id=" + std::to_string(doc_id)
            + ". Make sure Module 2 has processed this submission first.");

    std::string normalizedContent = r[0][0].c_str();
    std::vector<std::string> tokens;
    std::istringstream ss(normalizedContent);
    std::string token;
    while (ss >> token)
        tokens.push_back(token);

    return tokens;
}

bool PreprocessorDB::isProcessed(int doc_id) {
    pqxx::work txn(conn_);
    auto r = txn.exec(
        "SELECT 1 FROM PreprocessedDocs WHERE doc_id=$1",
        pqxx::params{doc_id});
    txn.commit();
    return !r.empty();
}

void PreprocessorDB::deleteResult(int doc_id) {
    pqxx::work txn(conn_);
    txn.exec(
        "DELETE FROM PreprocessedDocs WHERE doc_id=$1",
        pqxx::params{doc_id});
    txn.exec(
        "UPDATE Submissions SET status='PENDING' WHERE doc_id=$1",
        pqxx::params{doc_id});
    txn.commit();
}
