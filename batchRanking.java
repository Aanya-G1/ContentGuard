package com.contentguard.module7;
import com.contentguard.model.RawMatch;
import com.contentguard.model.RankedResult;
import java.util.*;
import java.util.stream.Collectors;
public class BatchRankingModule {
    private static final double EXACT_WEIGHT = 0.60;
    private static final double STRUCTURAL_WEIGHT = 0.40;
    private static final int TOP_MATCHES_LIMIT = 5;
    private static final double HIGH_THRESHOLD = 70.0;
    private static final double MEDIUM_THRESHOLD = 40.0;
    private static final double LOW_THRESHOLD = 10.0;
    public List<RankedResult> processBatch(List<RawMatch> rawMatches) {
        if (rawMatches == null || rawMatches.isEmpty()) {
            return Collections.emptyList();
        }
        Map<String, List<RawMatch>> grouped = groupBySubmission(rawMatches);
        PriorityQueue<RankedResult> maxHeap = new PriorityQueue<>();
        for (Map.Entry<String, List<RawMatch>> entry : grouped.entrySet()) {
            String submissionId = entry.getKey();
            List<RawMatch> matches = removeDuplicates(entry.getValue());
            double aggregatedScore = greedyAggregate(matches);
            List<RawMatch> topMatches = selectTopMatches(matches);
            String label = classifyLabel(aggregatedScore);
            RankedResult result = new RankedResult(
                    submissionId,
                    aggregatedScore,
                    label,
                    topMatches
            );
            maxHeap.offer(result);
        }
        return drainAndAssignRanks(maxHeap);
    }
    private Map<String, List<RawMatch>> groupBySubmission(List<RawMatch> matches) {
        return matches.stream()
                .collect(Collectors.groupingBy(RawMatch::getSubmissionId));
    }
    private List<RawMatch> removeDuplicates(List<RawMatch> matches) {
        Set<String> seen = new HashSet<>();
        List<RawMatch> unique = new ArrayList<>();
        for (RawMatch m : matches) {
            String key = m.getSubmissionId()
                    + m.getMatchedSourceId()
                    + m.getMatchedFragment();
            if (seen.add(key)) {
                unique.add(m);
            }
        }
        return unique;
    }
    private double greedyAggregate(List<RawMatch> matches) {
        matches.sort((a, b) -> Double.compare(
                combinedScore(b),
                combinedScore(a)
        ));
        double total = 0;
        int count = 0;
        for (RawMatch match : matches) {
            total += combinedScore(match);
            count++;
            if (count >= TOP_MATCHES_LIMIT) {
                break;
            }
        }
        if (count == 0) {
            return 0;
        }
        return Math.min(100.0, total / count);
    }
    private double combinedScore(RawMatch match) {
        return ((match.getExactScore() * EXACT_WEIGHT)
                + (match.getStructuralScore() * STRUCTURAL_WEIGHT)) * 100;
    }
    private List<RawMatch> selectTopMatches(List<RawMatch> matches) {
        return matches.stream()
                .sorted((a, b) -> Double.compare(combinedScore(b), combinedScore(a)))
                .limit(TOP_MATCHES_LIMIT)
                .collect(Collectors.toList());
    }
    private String classifyLabel(double score) {
        if (score >= HIGH_THRESHOLD) {
            return "HIGH";
        }
        if (score >= MEDIUM_THRESHOLD) {
            return "MEDIUM";
        }
        if (score >= LOW_THRESHOLD) {
            return "LOW";
        }
        return "NONE";
    }
    private List<RankedResult> drainAndAssignRanks(PriorityQueue<RankedResult> heap) {
        List<RankedResult> ranked = new ArrayList<>();
        int rank = 1;
        while (!heap.isEmpty()) {
            RankedResult result = heap.poll();
            result.setRank(rank++);
            ranked.add(result);
        }
        return ranked;
    }
}
