#pragma once
#include <string>
#include <vector>

enum class FileType { TEXT, CODE };

struct ProcessedDoc {
    int                      doc_id        = 0;   // ← renamed from submissionId
    int                      preprocessId = 0;
    std::string              normalizedContent;
    std::vector<std::string> tokens;
    int                      tokenCount   = 0;
    int                      charCount    = 0;
    FileType                 detectedType = FileType::TEXT;
};

class Preprocessor {
public:
    explicit Preprocessor(FileType type        = FileType::TEXT,
                          bool removeStopWords = true,
                          bool normalizeIds    = true);

    ProcessedDoc process(int doc_id, const std::string& rawContent) const;

    static FileType detectType(const std::string& fileName);

private:
    FileType fileType_;
    bool     removeStopWords_;
    bool     normalizeIds_;

    static std::string stripCodeComments(const std::string& src);
    static std::string normalizeIdentifiers(const std::string& src);
};
