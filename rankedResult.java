package com.contentguard.model;
import java.time.LocalDateTime;
import java.util.List;
public class RankedResult implements Comparable<RankedResult> {
    private String submissionId;
    private double aggregatedScore;
    private String plagiarismLabel;
    private int rank;
    private List<RawMatch> topMatches;
    private LocalDateTime processedAt;
    public RankedResult() {}
    public RankedResult(String submissionId,
                        double aggregatedScore,
                        String plagiarismLabel,
                        List<RawMatch> topMatches) {
        this.submissionId = submissionId;
        this.aggregatedScore = aggregatedScore;
        this.plagiarismLabel = plagiarismLabel;
        this.topMatches = topMatches;
        this.processedAt = LocalDateTime.now();
    }
    @Override
    public int compareTo(RankedResult other) {
        int cmp = Double.compare(other.aggregatedScore, this.aggregatedScore);
        if (cmp == 0) {
            return this.submissionId.compareTo(other.submissionId);
        }
        return cmp;
    }
    public String getSubmissionId() {
        return submissionId;
    }
    public void setSubmissionId(String submissionId) {
        this.submissionId = submissionId;
    }
    public double getAggregatedScore() {
        return aggregatedScore;
    }
    public void setAggregatedScore(double aggregatedScore) {
        if (aggregatedScore < 0 || aggregatedScore > 100) {
            throw new IllegalArgumentException("Aggregated score must be between 0 and 100");
        }
        this.aggregatedScore = aggregatedScore;
    }
    public String getPlagiarismLabel() {
        return plagiarismLabel;
    }
    public void setPlagiarismLabel(String plagiarismLabel) {
        this.plagiarismLabel = plagiarismLabel;
    }
    public int getRank() {
        return rank;
    }
    public void setRank(int rank) {
        this.rank = rank;
    }
    public List<RawMatch> getTopMatches() {
        return topMatches;
    }
    public void setTopMatches(List<RawMatch> topMatches) {
        this.topMatches = topMatches;
    }
    public LocalDateTime getProcessedAt() {
        return processedAt;
    }
    public void setProcessedAt(LocalDateTime processedAt) {
        this.processedAt = processedAt;
    }
    @Override
    public String toString() {
        return String.format(
                "RankedResult[rank=%d, submission=%s, score=%.2f%%, label=%s]",
                rank,
                submissionId,
                aggregatedScore,
                plagiarismLabel
        );
    }
}
