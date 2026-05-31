/**
 * ContentGuard — Token bridge for fenced StructuralSimilarity
 * Routes getTokensBydocId() to PreprocessorDB without changing LCS logic.
 */

#include "pipeline/token_bridge.hpp"
#include "preprocessor/preprocessor_db.hpp"

static PreprocessorDB* g_ppdb = nullptr;

void registerTokenBridge(PreprocessorDB* ppdb) {
    g_ppdb = ppdb;
}

std::vector<std::string> getTokensBydocId(int docId) {
    if (g_ppdb == nullptr) {
        return {};
    }
    return g_ppdb->getTokensByDocId(docId);
}
