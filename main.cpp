/**
 * ╔══════════════════════════════════════════════════════════╗
 * ║              ContentGuard System                         ║
 * ║  Module 1: User & Submission Management                  ║
 * ║  Module 2: Input Preprocessing & Normalization           ║
 * ╚══════════════════════════════════════════════════════════╝
 *
 * Build:  make
 * Run:    ./contentguard
 */

#include "module1/submission_manager.hpp"
#include "module2/preprocessor_db.hpp"
#include "db/db_config.hpp"
#include <iostream>

// ─────────────────────────────────────────────────────────────
// Helper: print a section separator
// ─────────────────────────────────────────────────────────────
static void separator(const std::string& title) {
    std::cout << "\n======================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "======================================\n";
}

// ─────────────────────────────────────────────────────────────
// safeAddUser
// Tries to register a user. If email already exists, fetches
// their existing userId instead of crashing.
// ─────────────────────────────────────────────────────────────
static int safeAddUser(SubmissionManager& mgr,
                       const std::string& name,
                       const std::string& email,
                       Role role) {
    try {
        int id = mgr.addUser(name, email, role);
        std::cout << "  Registered : " << name << " (userId=" << id << ")\n";
        return id;
    } catch (...) {
        // User already exists — fetch by email
        User u = mgr.getUserByEmail(email);
        std::cout << "  Exists     : " << name << " (userId=" << u.userId << ")\n";
        return u.userId;
    }
}

// ─────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────
int main() {
    std::cout << "\n======================================\n";
    std::cout << "       ContentGuard System\n";
    std::cout << "======================================\n";

    try {
        // ── Connect both modules to Neon ──────────────────────
        SubmissionManager mgr(DB::CONNECTION_STRING);
        PreprocessorDB    ppdb(DB::CONNECTION_STRING);
        std::cout << "\n✅ Connected to Neon PostgreSQL\n";

        // ══════════════════════════════════════════════════════
        // MODULE 1 — Register Users
        // ══════════════════════════════════════════════════════
        separator("MODULE 1 — Register Users");

        int uid1 = safeAddUser(mgr, "Aanya Godiyal",   "aanya@example.com",   Role::STUDENT);
        int uid2 = safeAddUser(mgr, "V Priya",          "priya@example.com",   Role::STUDENT);
        int uid3 = safeAddUser(mgr, "Drishti Painuli",  "drishti@example.com", Role::STUDENT);
        int uid4 = safeAddUser(mgr, "Yash Bahuguna",    "yash@example.com",    Role::STUDENT);
                   safeAddUser(mgr, "Prof. Sharma",     "sharma@example.com",  Role::INSTRUCTOR);

        std::cout << "\nAll users in database:\n";
        for (auto& u : mgr.listUsers(Role::STUDENT))
            std::cout << "  [Student]     userId=" << u.userId
                      << "  " << u.name << "\n";
        for (auto& u : mgr.listUsers(Role::INSTRUCTOR))
            std::cout << "  [Instructor]  userId=" << u.userId
                      << "  " << u.name << "\n";

        // ══════════════════════════════════════════════════════
        // MODULE 1 — Submit Files
        // ══════════════════════════════════════════════════════
        separator("MODULE 1 — Add Submissions");

        // Aanya's C++ code
        std::string cppCode1 =
            "#include <iostream>\n"
            "int main() {\n"
            "    int studentScore = 0;  // initialize score\n"
            "    studentScore = studentScore + 10;\n"
            "    /* compute final grade */\n"
            "    std::cout << studentScore;\n"
            "    return 0;\n"
            "}\n";

        // Priya's essay
        std::string essay =
            "Artificial intelligence is transforming modern education. "
            "Students now have access to personalized learning tools that adapt "
            "to their individual pace and style. The impact of AI on assessment "
            "and plagiarism detection is particularly significant in universities.";

        // Drishti's C++ code — same logic as Aanya but renamed variables
        std::string cppCode2 =
            "#include <iostream>\n"
            "int main() {\n"
            "    int gradePoint = 0;  // init grade\n"
            "    gradePoint = gradePoint + 10;\n"
            "    /* calculate total marks */\n"
            "    std::cout << gradePoint;\n"
            "    return 0;\n"
            "}\n";

        // Yash's Python code
        std::string pythonCode =
            "# Calculate student grade\n"
            "def calculate_grade(marks):\n"
            "    total = 0\n"
            "    total = total + marks\n"
            "    # print result\n"
            "    print(total)\n"
            "    return total\n";

        // Validate before submitting
        std::string err;
        auto validate = [&](const std::string& fn, const std::string& content) {
            if (!SubmissionManager::validateFile(fn, content, err)) {
                std::cerr << "  Rejected " << fn << ": " << err << "\n";
                return false;
            }
            return true;
        };

        int doc1 = -1, doc2 = -1, doc3 = -1, doc4 = -1;
        if (validate("solution.cpp",   cppCode1))  doc1 = mgr.addSubmission(uid1, "solution.cpp",   cppCode1);
        if (validate("essay.txt",      essay))      doc2 = mgr.addSubmission(uid2, "essay.txt",      essay);
        if (validate("solution2.cpp",  cppCode2))  doc3 = mgr.addSubmission(uid3, "solution2.cpp",  cppCode2);
        if (validate("grade_calc.py",  pythonCode)) doc4 = mgr.addSubmission(uid4, "grade_calc.py",  pythonCode);

        std::cout << "Submissions saved:\n";
        if (doc1 != -1) std::cout << "  doc_id=" << doc1 << "  solution.cpp   (Aanya)   -> CODE\n";
        if (doc2 != -1) std::cout << "  doc_id=" << doc2 << "  essay.txt      (Priya)   -> TEXT\n";
        if (doc3 != -1) std::cout << "  doc_id=" << doc3 << "  solution2.cpp  (Drishti) -> CODE\n";
        if (doc4 != -1) std::cout << "  doc_id=" << doc4 << "  grade_calc.py  (Yash)    -> CODE\n";

        // ── Queue all for Module 2 ────────────────────────────
        if (doc1 != -1) mgr.updateStatus(doc1, SubmissionStatus::PROCESSING);
        if (doc2 != -1) mgr.updateStatus(doc2, SubmissionStatus::PROCESSING);
        if (doc3 != -1) mgr.updateStatus(doc3, SubmissionStatus::PROCESSING);
        if (doc4 != -1) mgr.updateStatus(doc4, SubmissionStatus::PROCESSING);
        std::cout << "\nAll submissions queued -> status=PROCESSING\n";

        // ══════════════════════════════════════════════════════
        // MODULE 1 — Show submission by status
        // ══════════════════════════════════════════════════════
        separator("MODULE 1 — Submissions by Status");
        auto processing = mgr.getSubmissionsByStatus(SubmissionStatus::PROCESSING);
        std::cout << "Submissions with status=PROCESSING: "
                  << processing.size() << "\n";
        for (auto& s : processing)
            std::cout << "  doc_id=" << s.doc_id
                      << "  " << s.fileName
                      << "  (" << (s.type==SubmissionType::CODE?"CODE":"TEXT") << ")\n";

        // ══════════════════════════════════════════════════════
        // MODULE 2 — Preprocess All Pending
        // ══════════════════════════════════════════════════════
        separator("MODULE 2 — Preprocessing Pipeline");

        int processed = ppdb.processAllPending();
        std::cout << "\n✅ " << processed << " submission(s) preprocessed\n";

        // ══════════════════════════════════════════════════════
        // MODULE 2 — Show Results
        // ══════════════════════════════════════════════════════
        separator("MODULE 2 — Preprocessed Results");

        auto results = ppdb.getAllResults();
        for (auto& doc : results) {
            std::cout << "\n  preprocessId=" << doc.preprocessId
                      << "  doc_id=" << doc.doc_id << "\n";
            std::cout << "  Tokens  : " << doc.tokenCount << "\n";
            std::cout << "  Chars   : " << doc.charCount  << "\n";
            std::cout << "  Type    : "
                      << (doc.detectedType == FileType::CODE ? "CODE" : "TEXT") << "\n";
            // Show first 80 chars of normalized content
            std::string preview = doc.normalizedContent.substr(
                0, std::min((int)doc.normalizedContent.size(), 80));
            std::cout << "  Preview : " << preview << "...\n";
        }

        // ══════════════════════════════════════════════════════
        // MODULE 2 — Demo: getTokensByDocId for Module 3
        // ══════════════════════════════════════════════════════
        separator("MODULE 2 — getTokensByDocId Demo (for Module 3)");

        if (doc1 != -1 && ppdb.isProcessed(doc1)) {
            auto tokens = ppdb.getTokensByDocId(doc1);
            std::cout << "Tokens for doc_id=" << doc1
                      << " (Aanya's solution.cpp):\n";
            std::cout << "  Count: " << tokens.size() << "\n";
            std::cout << "  Tokens: ";
            for (size_t i = 0; i < std::min(tokens.size(), (size_t)15); ++i)
                std::cout << "[" << tokens[i] << "] ";
            std::cout << "...\n";
        }

        if (doc3 != -1 && ppdb.isProcessed(doc3)) {
            auto tokens = ppdb.getTokensByDocId(doc3);
            std::cout << "\nTokens for doc_id=" << doc3
                      << " (Drishti's solution2.cpp):\n";
            std::cout << "  Count: " << tokens.size() << "\n";
            std::cout << "  Tokens: ";
            for (size_t i = 0; i < std::min(tokens.size(), (size_t)15); ++i)
                std::cout << "[" << tokens[i] << "] ";
            std::cout << "...\n";
        }

        // ══════════════════════════════════════════════════════
        // Plagiarism Proof — show identical normalization
        // ══════════════════════════════════════════════════════
        separator("Plagiarism Detection Proof");

        if (doc1 != -1 && doc3 != -1 &&
            ppdb.isProcessed(doc1) && ppdb.isProcessed(doc3)) {
            auto d1 = ppdb.getResultByDocId(doc1);
            auto d3 = ppdb.getResultByDocId(doc3);
            std::cout << "Aanya's   normalizedContent: "
                      << d1.normalizedContent.substr(0,60) << "\n";
            std::cout << "Drishti's normalizedContent: "
                      << d3.normalizedContent.substr(0,60) << "\n";
            if (d1.normalizedContent == d3.normalizedContent)
                std::cout << "\n✅ IDENTICAL — plagiarism will be detected by Module 4!\n";
            else
                std::cout << "\n⚠️  Different normalized content.\n";
        }

        // ══════════════════════════════════════════════════════
        // Handoff Summary for Module 3
        // ══════════════════════════════════════════════════════
        separator("Handoff -> Module 3 (Fingerprint Generation)");
        std::cout << "  PreprocessedDocs table has " << results.size()
                  << " document(s) ready.\n";
        std::cout << "  Module 3 calls: ppdb.getTokensByDocId(doc_id)\n";
        std::cout << "  to get tokens for Rolling Hash fingerprinting.\n";

        std::cout << "\n✅ Modules 1 & 2 complete.\n\n";

    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        std::cerr << "   -> Check db/db_config.hpp with your Neon connection string.\n";
        return 1;
    }
    return 0;
}
