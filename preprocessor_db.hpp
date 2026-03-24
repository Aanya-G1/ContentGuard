#pragma once
#include "preprocessor.hpp"
#include <pqxx/pqxx>
#include <string>
#include <vector>

class PreprocessorDB {
public:
    explicit PreprocessorDB(const std::string& connString);

    /** Process one submission by its doc_id. Returns new preprocessId. */
    int processSubmission(int doc_id);

    /** Process all submissions with status=PROCESSING. Returns count. */
    int processAllPending();

    /** Fetch a saved result by preprocessId. */
    ProcessedDoc getResult(int preprocessId);

    /** Get all preprocessed docs (for Module 3). */
    std::vector<ProcessedDoc> getAllResults();

private:
    pqxx::connection conn_;
};
