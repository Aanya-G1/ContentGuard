#include "preprocessor_db.hpp"
#include <iostream>
#include <stdexcept>

PreprocessorDB::PreprocessorDB(const std::string& connString)
    : conn_(connString) {}

int PreprocessorDB::processSubmission(int doc_id) {
    // Step 1: Read submission using doc_id
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT content, fileName, fileType"
        " FROM Submissions WHERE doc_id=$1",
        doc_id);
    txn.commit();

    if (r.empty())
        throw std::runtime_error("Submission not found: doc_id=" + std::to_string(doc_id));

    std::string content  = r[0][0].c_str();
    std::string fileName = r[0][1].c_str();
    std::string fileType = r[0][2].c_str();

    // Step 2: Run preprocessing pipeline
    FileType ft = (fileType == "CODE") ? FileType::CODE : FileType::TEXT;
    Preprocessor pp(ft, true, true);
    ProcessedDoc doc = pp.process(doc_id, content);

    std::string detectedStr = (doc.detectedType == FileType::CODE) ? "CODE" : "TEXT";

    // Step 3: Save to PreprocessedDocs using doc_id
    pqxx::work txn2(conn_);
    auto res = txn2.exec_params(
        "INSERT INTO PreprocessedDocs"
        "  (doc_id, normalizedContent, tokenCount, charCount, detectedType)"
        "  VALUES($1,$2,$3,$4,$5)"
        "  RETURNING preprocessId",
        doc_id,
        doc.normalizedContent,
        doc.tokenCount,
        doc.charCount,
        detectedStr);

    // Step 4: Update status to DONE
    txn2.exec_params(
        "UPDATE Submissions SET status='DONE' WHERE doc_id=$1", doc_id);

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
        "SELECT doc_id FROM Submissions WHERE status='PROCESSING'");
    txn.commit();

    if (rows.empty()) {
        std::cout << "  [Module 2] No submissions in PROCESSING queue.\n";
        return 0;
    }

    int count = 0;
    for (const auto& row : rows) {
        try {
            processSubmission(row[0].as<int>());
            ++count;
        } catch (const std::exception& e) {
            std::cerr << "  [Module 2] ERROR on doc_id="
                      << row[0].as<int>() << ": " << e.what() << "\n";
            try {
                pqxx::work fix(conn_);
                fix.exec_params(
                    "UPDATE Submissions SET status='ERROR' WHERE doc_id=$1",
                    row[0].as<int>());
                fix.commit();
            } catch (...) {}
        }
    }
    return count;
}

ProcessedDoc PreprocessorDB::getResult(int preprocessId) {
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT preprocessId, doc_id, normalizedContent,"
        "       tokenCount, charCount, detectedType"
        " FROM PreprocessedDocs WHERE preprocessId=$1",
        preprocessId);
    txn.commit();

    if (r.empty())
        throw std::runtime_error("Result not found: preprocessId=" + std::to_string(preprocessId));

    ProcessedDoc doc;
    doc.preprocessId      = r[0][0].as<int>();
    doc.doc_id             = r[0][1].as<int>();
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
        doc.doc_id             = row[1].as<int>();
        doc.normalizedContent = row[2].c_str();
        doc.tokenCount        = row[3].as<int>();
        doc.charCount         = row[4].as<int>();
        doc.detectedType      = (std::string(row[5].c_str()) == "CODE")
                                ? FileType::CODE : FileType::TEXT;
        results.push_back(doc);
    }
    return results;
}
