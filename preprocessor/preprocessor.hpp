#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// ─────────────────────────────────────────────────────────────
// FileType — TEXT or CODE
// ─────────────────────────────────────────────────────────────
enum class FileType { TEXT, CODE };

// ─────────────────────────────────────────────────────────────
// ProcessedDoc — output of the preprocessing pipeline
// This is what gets saved to PreprocessedDocs table
// and what Module 3 reads for fingerprinting
// ─────────────────────────────────────────────────────────────
struct ProcessedDoc {
    int                      doc_id        = 0;   // links to Submissions table
    int                      preprocessId  = 0;   // auto-assigned by DB
    std::string              normalizedContent;   // cleaned, lowercase, space-separated tokens
    std::vector<std::string> tokens;              // individual tokens as a vector
    int                      tokenCount    = 0;
    int                      charCount     = 0;
    FileType                 detectedType  = FileType::TEXT;
};

// ─────────────────────────────────────────────────────────────
// Preprocessor — Module 2 core pipeline (no DB dependency)
// Cleans raw file content through a multi-step pipeline
// ─────────────────────────────────────────────────────────────
class Preprocessor {
public:
    /**
     * @param type             FILE type — TEXT or CODE
     * @param removeStopWords  remove common English words (TEXT only)
     * @param normalizeIds     replace variable names with VAR_N (CODE only)
     */
    explicit Preprocessor(FileType type        = FileType::TEXT,
                          bool removeStopWords = true,
                          bool normalizeIds    = true);

    /**
     * Run the full preprocessing pipeline on raw content.
     * @param doc_id     the document ID from Submissions table
     * @param rawContent the raw file content to process
     * @return           ProcessedDoc with tokens and normalizedContent
     */
    ProcessedDoc process(int doc_id, const std::string& rawContent) const;

    /**
     * Auto-detect file type from file extension.
     * .cpp, .py, .java etc → CODE
     * .txt, .pdf, .md etc  → TEXT
     */
    static FileType detectType(const std::string& fileName);

private:
    FileType fileType_;
    bool     removeStopWords_;
    bool     normalizeIds_;

    // ── Pipeline Steps (CODE only) ────────────────────────────
    /** Remove line comments, block comments, and hash-style comments */
    static std::string stripCodeComments(const std::string& src);

    /** Replace variable/function names with VAR_0, VAR_1...
     *  Language keywords (int, if, return, for...) are preserved */
    static std::string normalizeIdentifiers(const std::string& src);
};
