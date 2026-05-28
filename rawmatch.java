package com.contentguard.model;
import java.util.Objects;
public class RawMatch {
    private String submissionId;
    private String matchedSourceId;
    private double exactScore;
    private double structuralScore;
    private String matchedFragment;
    private int matchStartIndex;
    private int matchEndIndex;
    public RawMatch() {}
    public RawMatch(String submissionId,
                    String matchedSourceId,
                    double exactScore,
                    double structuralScore,
                    String matchedFragment,
                    int matchStartIndex,
                    int matchEndIndex) {
        this.submissionId = Objects.requireNonNull(submissionId);
        this.matchedSourceId = Objects.requireNonNull(matchedSourceId);
        setExactScore(exactScore);
        setStructuralScore(structuralScore);
        this.matchedFragment = matchedFragment;
        this.matchStartIndex = matchStartIndex;
        this.matchEndIndex = matchEndIndex;
    }
    public String getSubmissionId() {
        return submissionId;
    }
    public void setSubmissionId(String submissionId) {
        this.submissionId = submissionId;
    }
    public String getMatchedSourceId() {
        return matchedSourceId;
    }
    public void setMatchedSourceId(String matchedSourceId) {
        this.matchedSourceId = matchedSourceId;
    }
    public double getExactScore() {
        return exactScore;
    }
    public void setExactScore(double exactScore) {
        validateScore(exactScore, "Exact score");
        this.exactScore = exactScore;
    }
    public double getStructuralScore() {
        return structuralScore;
    }
    public void setStructuralScore(double structuralScore) {
        validateScore(structuralScore, "Structural score");
        this.structuralScore = structuralScore;
    }
    public String getMatchedFragment() {
        return matchedFragment;
    }
    public void setMatchedFragment(String matchedFragment) {
        this.matchedFragment = matchedFragment;
    }
    public int getMatchStartIndex() {
        return matchStartIndex;
    }
    public void setMatchStartIndex(int matchStartIndex) {
        this.matchStartIndex = matchStartIndex;
    }
    public int getMatchEndIndex() {
        return matchEndIndex;
    }
    public void setMatchEndIndex(int matchEndIndex) {
        this.matchEndIndex = matchEndIndex;
    }
    private void validateScore(double value, String field) {
        if (value < 0.0 || value > 1.0) {
            throw new IllegalArgumentException(field + " must be between 0 and 1");
        }
    }
    @Override
    public String toString() {
        return String.format(
                "RawMatch[submission=%s, source=%s, exact=%.2f, structural=%.2f]",
                submissionId,
                matchedSourceId,
                exactScore,
                structuralScore
        );
    }
}
