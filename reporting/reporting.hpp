#pragma once

/**
 * Step 5 — Reporting (unfenced)
 * Formats pipeline results as JSON for the HTML/JS frontend.
 */

#include "json.hpp"
#include "pipeline/pipeline.hpp"
#include <vector>

class ReportingModule {
public:
  /** Full scan report for one submission. */
  static nlohmann::json buildReportJson(const ScanReport& report);

  /** Submit API response envelope. */
  static nlohmann::json buildSubmitResponse(const ScanReport& report,
                                            const nlohmann::json& metadata);

  /** Ranked list across a batch (for documents view / batch export). */
  static nlohmann::json buildRankedListJson(
      const std::vector<ScanReport>& reports);
};
