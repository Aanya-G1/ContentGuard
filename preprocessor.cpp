#include "preprocessor.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>
#include <unordered_set>
#include <unordered_map>

// ── Stop words (TEXT mode) ────────────────────────────────────
static const std::unordered_set<std::string> STOP_WORDS = {
    "a","an","the","is","it","in","on","at","to","of","and","or","but",
    "for","with","as","by","from","this","that","these","those","was",
    "were","are","be","been","being","have","has","had","do","does","did",
    "will","would","shall","should","may","might","can","could","not","no",
    "if","then","else","while","so","such","each","both","few","more",
    "most","other","some","than","too","very","just","also","into","its"
};

// ── Programming keywords (never replaced in CODE mode) ────────
static const std::unordered_set<std::string> CODE_KEYWORDS = {
    "int","float","double","char","bool","void","string","auto","return",
    "if","else","while","for","do","break","continue","class","struct",
    "public","private","protected","new","delete","nullptr","true","false",
    "include","define","namespace","using","template","typename","const",
    "static","extern","virtual","override","final","import","def","lambda",
    "try","catch","throw","switch","case","default","sizeof","typedef","print"
};

static const std::unordered_set<std::string> CODE_EXTS = {
    ".cpp",".cc",".c",".h",".hpp",".java",".py",".js",
    ".ts",".cs",".rb",".go",".rs",".swift",".kt",".php"
};

static std::string toLower(const std::string& s) {
    std::string o = s;
    std::transform(o.begin(), o.end(), o.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return o;
}

// ── Constructor ───────────────────────────────────────────────
Preprocessor::Preprocessor(FileType type, bool removeStopWords, bool normalizeIds)
    : fileType_(type), removeStopWords_(removeStopWords), normalizeIds_(normalizeIds) {}

// ── Auto detect file type from extension ─────────────────────
FileType Preprocessor::detectType(const std::string& fileName) {
    auto dot = fileName.rfind('.');
    if (dot == std::string::npos) return FileType::TEXT;
    return CODE_EXTS.count(toLower(fileName.substr(dot)))
           ? FileType::CODE : FileType::TEXT;
}

// ── Strip comments from code ──────────────────────────────────
std::string Preprocessor::stripCodeComments(const std::string& src) {
    std::string out;
    out.reserve(src.size());
    bool inBlock = false;
    size_t i = 0;
    while (i < src.size()) {
        if (!inBlock && i+1 < src.size() && src[i]=='/' && src[i+1]=='*')
            { inBlock = true;  i += 2; }
        else if (inBlock && i+1 < src.size() && src[i]=='*' && src[i+1]=='/')
            { inBlock = false; i += 2; }
        else if (!inBlock && i+1 < src.size() && src[i]=='/' && src[i+1]=='/')
            { while (i < src.size() && src[i] != '\n') ++i; }
        else if (!inBlock && src[i] == '#')
            { while (i < src.size() && src[i] != '\n') ++i; }
        else {
            if (!inBlock) out += src[i];
            ++i;
        }
    }
    return out;
}

// ── Replace variable names with VAR_0, VAR_1 ... ─────────────
std::string Preprocessor::normalizeIdentifiers(const std::string& src) {
    static const std::regex identRe(R"(\b([a-zA-Z_][a-zA-Z0-9_]*)\b)");
    std::unordered_map<std::string, std::string> mapping;
    int counter = 0;
    std::string out;
    size_t lastPos = 0;
    auto it  = std::sregex_iterator(src.begin(), src.end(), identRe);
    auto end = std::sregex_iterator();
    for (; it != end; ++it) {
        auto& m = *it;
        out += src.substr(lastPos, m.position() - lastPos);
        std::string tok = m.str();
        if (CODE_KEYWORDS.count(tok)) {
            out += tok;
        } else {
            if (!mapping.count(tok))
                mapping[tok] = "VAR_" + std::to_string(counter++);
            out += mapping[tok];
        }
        lastPos = m.position() + m.length();
    }
    out += src.substr(lastPos);
    return out;
}

// ── Main Pipeline ─────────────────────────────────────────────
// doc_id replaces submissionId — passed in from the DB layer
ProcessedDoc Preprocessor::process(int doc_id,
                                    const std::string& rawContent) const {
    ProcessedDoc doc;
    doc.doc_id = doc_id;   // ← using doc_id now

    std::string working = rawContent;

    // Step 1: Code only — strip comments and normalize identifiers
    if (fileType_ == FileType::CODE) {
        working = stripCodeComments(working);
        if (normalizeIds_) working = normalizeIdentifiers(working);
    }

    // Step 2: Lowercase everything
    working = toLower(working);

    // Step 3: Remove punctuation
    std::string clean;
    clean.reserve(working.size());
    for (unsigned char c : working)
        clean += (std::isalnum(c) || std::isspace(c)) ? c : ' ';

    // Step 4: Tokenize by whitespace
    std::istringstream ss(clean);
    std::string token;
    while (ss >> token) {
        if (token.empty()) continue;
        // Step 5: Remove stop words (TEXT mode only)
        if (removeStopWords_ && fileType_ == FileType::TEXT
            && STOP_WORDS.count(token))
            continue;
        doc.tokens.push_back(token);
    }

    // Step 6: Build normalized string
    std::ostringstream joined;
    for (size_t i = 0; i < doc.tokens.size(); ++i) {
        if (i) joined << ' ';
        joined << doc.tokens[i];
    }
    doc.normalizedContent = joined.str();
    doc.tokenCount        = (int)doc.tokens.size();
    doc.charCount         = (int)doc.normalizedContent.size();
    doc.detectedType      = fileType_;

    return doc;
}
