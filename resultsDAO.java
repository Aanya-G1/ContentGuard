package com.contentguard.db;
import com.contentguard.model.RawMatch;
import com.contentguard.model.RankedResult;
import java.sql.*;
import java.util.ArrayList;
import java.util.List;
public class ResultsDAO {
    private final Connection connection;
    public ResultsDAO(Connection connection) {
        this.connection = connection;
    }
    public void saveRankedResults(List<RankedResult> rankedResults,
                                  String batchId) throws SQLException {
        String sql = "INSERT INTO ranked_results " +
                "(submission_id, aggregated_score, plagiarism_label, rank_position, batch_id, processed_at) " +
                "VALUES (?, ?, ?, ?, ?, ?) " +
                "ON CONFLICT (submission_id, batch_id) DO UPDATE " +
                "SET aggregated_score = EXCLUDED.aggregated_score, " +
                "plagiarism_label = EXCLUDED.plagiarism_label, " +
                "rank_position = EXCLUDED.rank_position";
        try (PreparedStatement stmt = connection.prepareStatement(sql)) {
            connection.setAutoCommit(false);
            for (RankedResult r : rankedResults) {
                stmt.setString(1, r.getSubmissionId());
                stmt.setDouble(2, r.getAggregatedScore());
                stmt.setString(3, r.getPlagiarismLabel());
                stmt.setInt(4, r.getRank());
                stmt.setString(5, batchId);
                stmt.setTimestamp(6, Timestamp.valueOf(r.getProcessedAt()));
                stmt.addBatch();
            }
            stmt.executeBatch();
            connection.commit();
        } catch (SQLException e) {
            try {
                connection.rollback();
            } catch (SQLException rollbackEx) {
                rollbackEx.printStackTrace();
            }
            throw e;
        } finally {
            connection.setAutoCommit(true);
        }
    }
    public List<RawMatch> loadRawMatchesForBatch(String batchId)
            throws SQLException {
        String sql = "SELECT submission_id, matched_source_id, " +
                "exact_score, structural_score, matched_fragment, " +
                "match_start_index, match_end_index " +
                "FROM raw_matches WHERE batch_id = ?";
        List<RawMatch> matches = new ArrayList<>();
        try (PreparedStatement stmt = connection.prepareStatement(sql)) {
            stmt.setString(1, batchId);
            try (ResultSet rs = stmt.executeQuery()) {
                while (rs.next()) {
                    RawMatch match = new RawMatch(
                            rs.getString("submission_id"),
                            rs.getString("matched_source_id"),
                            rs.getDouble("exact_score"),
                            rs.getDouble("structural_score"),
                            rs.getString("matched_fragment"),
                            rs.getInt("match_start_index"),
                            rs.getInt("match_end_index")
                    );
                    matches.add(match);
                }
            }
        }
        return matches;
    }
}
