#ifndef FINGERPRINT_MODULE_H
#define FINGERPRINT_MODULE_H

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <pqxx/pqxx>
#include <deque>
#include <algorithm>

class FingerPrintGeneration {
private:
    int k;
    std::vector<uint64_t> hashes;
    const uint64_t base = 31;
    const uint64_t mod = 1e9 + 7;
    uint64_t h;

    uint64_t tokenToInt(const std::string& token);

public:
    FingerPrintGeneration(int k_val);
    std::vector<std::vector<std::string>> kgramGeneration(std::vector<std::string>& tokens);
    void generateHashes(std::vector<std::vector<std::string>> kgrams);
    uint64_t rollinghashGeneration(uint64_t prevHash, uint64_t OutVal, uint64_t InVal);
    std::vector<uint64_t> getHashes() const;
    std::vector<uint64_t> winnowing(const std::vector<uint64_t>& hashes);
};

class DatabaseOperations {
private:
    std::unique_ptr<pqxx::connection> conn;

public:
    void Connection(); 
    void storeFingerprints(int docId, const std::vector<uint64_t>& hashes);
    std::vector<uint64_t> getFingerprints(int docId);
    std::vector<int> findMatch(const std::vector<uint64_t>& hashes);
};

std::vector<int> fingerprintProcessing(int docId,std::vector<std::string> tokens);
#endif