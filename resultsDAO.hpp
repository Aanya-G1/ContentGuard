#ifndef RESULTSDAO_HPP
#define RESULTSDAO_HPP
#include <pqxx/pqxx>
#include <vector>
#include <iostream>
using namespace std;
#include "RawMatch.hpp"
#include "RankedResult.hpp"
class ResultsDAO {
private:
    pqxx::connection& conn;
public:
    ResultsDAO(pqxx::connection& connection)
        : conn(connection) {}
    void saveRankedResults(
        const vector<RankedResult>& results,
        const string& batchId
    ) {
        try {
            pqxx::work txn(conn);
            for (const auto& result : results) {
                txn.exec_params(
                    "INSERT INTO ranked_results "
                    "(submission_id, aggregated_score, plagiarism_label, rank_position, batch_id) "
                    "VALUES ($1, $2, $3, $4, $5)",
                    result.getSubmissionId(),
                    result.getAggregatedScore(),
                    result.getPlagiarismLabel(),
                    result.getRank(),
                    batchId
                );
            }
            txn.commit();
        } catch (const exception& e) {
            cerr << "Database Error: " << e.what() << endl;
        }
    }
};
#endif
