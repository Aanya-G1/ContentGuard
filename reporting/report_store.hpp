#pragma once

#include "pipeline/pipeline.hpp"
#include <string>
#include <unordered_map>

/** Persists ScanReport JSON to PostgreSQL (survives server restart). */
class ReportStore {
public:
    explicit ReportStore(const std::string& connString);

    void save(const ScanReport& report);
    bool load(int docId, ScanReport& out) const;
    void remove(int docId);
    void clearAll();

    void loadAll(std::unordered_map<int, ScanReport>& out) const;

private:
    std::string connString_;
};
