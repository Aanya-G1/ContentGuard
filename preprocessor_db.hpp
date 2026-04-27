#pragma once
#include "preprocessor.hpp"
#include <pqxx/pqxx>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────
// PreprocessorDB — Module 2 Database Layer
//
// Connects the preprocessing pipeline to Neon PostgreSQL.
//
// Workflow:
//   Module 1 sets submission status → PROCESSING
//   PreprocessorDB reads those submissions
//   Runs Preprocessor pipeline on each
//   Saves result to PreprocessedDocs table
//   Sets submission status → DONE
//
// Output feeds into Module 3 (Fingerprint Generation)
// via getTokensByDocId()
// ─────────────────────────────────────────────────────────────
class PreprocessorDB {
public:
    explicit PreprocessorDB(const std::string& connString);

    // ── Processing ────────────────────────────────────────────

    /**
     * Process a single submission by doc_id.
     * Reads from Submissions table, runs pipeline,
     * saves to PreprocessedDocs, sets status=DONE.
     * @return new preprocessId assigned by DB
     */
    int processSubmission(int doc_id);

    /**
     * Process ALL submissions with status='PROCESSING'.
     * Calls processSubmission() on each.
     * If one fails, marks it ERROR and continues.
     * @return number of submissions successfully processed
     */
    int processAllPending();

    // ── Retrieval ─────────────────────────────────────────────

    /**
     * Fetch a saved preprocessed result by preprocessId.
     * @throws if not found
     */
    ProcessedDoc getResult(int preprocessId);

    /**
     * Fetch a saved preprocessed result by doc_id.
     * @throws if not found
     */
    ProcessedDoc getResultByDocId(int doc_id);

    /**
     * Get ALL preprocessed documents.
     * Used by Module 3 to get all docs for fingerprinting.
     */
    std::vector<ProcessedDoc> getAllResults();

    /**
     * getTokensByDocId — PRIMARY HANDOFF FUNCTION FOR MODULE 3
     *
     * Given a doc_id, returns the list of cleaned tokens
     * for that document from the PreprocessedDocs table.
     *
     * Module 3 calls this to get tokens for Rolling Hash fingerprinting.
     *
     * Example:
     *   doc_id=13 (Aanya's solution.cpp) returns:
     *   ["int","var","0","var","1","0","var","1","return","0"]
     *
     * @param doc_id  the document ID from Submissions table
     * @return        vector<string> of tokens
     * @throws        if doc_id has no preprocessed result yet
     */
    std::vector<std::string> getTokensByDocId(int doc_id);

    // ── Utility ───────────────────────────────────────────────

    /**
     * Check if a doc_id has already been preprocessed.
     * Useful to avoid double-processing.
     */
    bool isProcessed(int doc_id);

    /**
     * Delete a preprocessed result by doc_id.
     * Used if re-processing is needed.
     */
    void deleteResult(int doc_id);

private:
    pqxx::connection conn_;
};
