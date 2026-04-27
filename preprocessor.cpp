#include "preprocessor.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>
#include <unordered_set>

// ─────────────────────────────────────────────────────────────
// Stop Words — common English words removed from TEXT files
// These appear in every essay and carry no similarity meaning
// ─────────────────────────────────────────────────────────────
static const std::unordered_set<std::string> STOP_WORDS = {
    "a","an","the","is","it","in","on","at","to","of","and","or","but",
    "for","with","as","by","from","this","that","these","those","was",
    "were","are","be","been","being","have","has","had","do","does","did",
    "will","would","shall","should","may","might","can","could","not","no",
    "if","then","else","while","so","such","each","both","few","more",
    "most","other","some","than","too","very","just","also","into","its",
    "our","your","my","his","her","their","we","they","you","i","he","she",
    "us","him","about","up","out","which","who","what","when","where","how"
};

// ─────────────────────────────────────────────────────────────
// Code Keywords — never replaced during identifier normalization
// These are part of language structure, not variable names
// ─────────────────────────────────────────────────────────────
static const std::unordered_set<std::string> CODE_KEYWORDS = {
    // C++ / Java / C
    "int","float","double","char","bool","void","string","auto","return",
    "if","else","while","for","do","break","continue","class","struct",
    "public","private","protected","new","delete","nullptr","true","false",
    "include","define","namespace","using","template","typename","const",
    "static","extern","virtual","override","final","sizeof","typedef",
    "try","catch","throw","switch","case","default","enum","union",
    // Python
    "def","lambda","import","from","pass","raise","with","yield","global",
    "nonlocal","assert","del","except","finally","in","not","and","or","is",
    "print","range","len","input","type","list","dict","set","tuple",
    // JavaScript / TypeScript
    "var","let","const","function","=>","async","await","typeof","instanceof",
    // General
    "main","cout","cin","endl","std","printf","scanf","return"
};

// ─────────────────────────────────────────────────────────────
// Code file extensions
// ─────────────────────────────────────────────────────────────
static const std::unordered_set<std::string> CODE_EXTS = {
    ".cpp",".cc",".c",".h",".hpp",".java",".py",".js",
    ".ts",".cs",".rb",".go",".rs",".swift",".kt",".php"
};

// ─────────────────────────────────────────────────────────────
// Helper: lowercase a string
// ─────────────────────────────────────────────────────────────
static std::string toLower(const std::string& s) {
    std::string o = s;
    std::transform(o.begin(), o.end(), o.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return o;
}

// ─────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────
Preprocessor::Preprocessor(FileType type, bool removeStopWords, bool normalizeIds)
    : fileType_(type), removeStopWords_(removeStopWords), normalizeIds_(normalizeIds) {}

// ─────────────────────────────────────────────────────────────
// detectType — auto-detect from file extension
// ─────────────────────────────────────────────────────────────
FileType Preprocessor::detectType(const std::string& fileName) {
    auto dot = fileName.rfind('.');
    if (dot == std::string::npos) return FileType::TEXT;
    return CODE_EXTS.count(toLower(fileName.substr(dot)))
           ? FileType::CODE : FileType::TEXT;
}

// ─────────────────────────────────────────────────────────────
// stripCodeComments
// Removes: // single line, /* block */, # Python comments
// ─────────────────────────────────────────────────────────────
std::string Preprocessor::stripCodeComments(const std::string& src) {
    std::string out;
    out.reserve(src.size());
    bool inBlock = false;
    size_t i = 0;
    while (i < src.size()) {
        // Start of block comment /*
        if (!inBlock && i+1 < src.size() && src[i]=='/' && src[i+1]=='*') {
            inBlock = true; i += 2;
        }
        // End of block comment */
        else if (inBlock && i+1 < src.size() && src[i]=='*' && src[i+1]=='/') {
            inBlock = false; i += 2;
        }
        // Single line comment //
        else if (!inBlock && i+1 < src.size() && src[i]=='/' && src[i+1]=='/') {
            while (i < src.size() && src[i] != '\n') ++i;
        }
        // Python / shell comment #
        else if (!inBlock && src[i] == '#') {
            while (i < src.size() && src[i] != '\n') ++i;
        }
        else {
            if (!inBlock) out += src[i];
            ++i;
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────
// normalizeIdentifiers
// Replaces user-defined names with VAR_0, VAR_1, VAR_2...
// Same identifier always maps to same VAR_N within one file
// Keywords are preserved exactly as-is
// ─────────────────────────────────────────────────────────────
std::string Preprocessor::normalizeIdentifiers(const std::string& src) {
    // Match any valid identifier: starts with letter or _, followed by letters/digits/_
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
            // Keep keywords exactly as-is
            out += tok;
        } else {
            // Map user identifier to VAR_N (same name → same VAR_N)
            if (!mapping.count(tok))
                mapping[tok] = "VAR_" + std::to_string(counter++);
            out += mapping[tok];
        }
        lastPos = m.position() + m.length();
    }
    out += src.substr(lastPos);
    return out;
}

// ─────────────────────────────────────────────────────────────
// process — MAIN PIPELINE
//
// For TEXT files (5 steps):
//   1. Lowercase
//   2. Remove punctuation
//   3. Tokenize
//   4. Remove stop words
//   5. Build normalized string
//
// For CODE files (2 extra steps first):
//   1. Strip comments
//   2. Normalize identifiers (VAR_0, VAR_1...)
//   then same 5 steps as TEXT
// ─────────────────────────────────────────────────────────────
ProcessedDoc Preprocessor::process(int doc_id,
                                    const std::string& rawContent) const {
    ProcessedDoc doc;
    doc.doc_id = doc_id;

    std::string working = rawContent;

    // ── CODE-only steps ───────────────────────────────────────
    if (fileType_ == FileType::CODE) {
        // Step 1: Remove all comments
        working = stripCodeComments(working);

        // Step 2: Replace variable names with VAR_N
        if (normalizeIds_) working = normalizeIdentifiers(working);
    }

    // ── Step 3: Lowercase everything ─────────────────────────
    working = toLower(working);

    // ── Step 4: Remove punctuation (replace with space) ───────
    std::string clean;
    clean.reserve(working.size());
    for (unsigned char c : working)
        clean += (std::isalnum(c) || std::isspace(c)) ? c : ' ';

    // ── Step 5: Tokenize by whitespace ────────────────────────
    std::istringstream ss(clean);
    std::string token;
    while (ss >> token) {
        if (token.empty()) continue;
        // ── Step 6: Remove stop words (TEXT only) ─────────────
        if (removeStopWords_ && fileType_ == FileType::TEXT
            && STOP_WORDS.count(token))
            continue;
        doc.tokens.push_back(token);
    }

    // ── Step 7: Build normalized content string ───────────────
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
