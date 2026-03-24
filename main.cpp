/**
 * ContentGuard – Main Entry Point
 * Module 1: User & Submission Management
 * Module 2: Input Preprocessing & Normalization
 * (submissionId renamed to doc_id everywhere)
 */

#include "module1/submission_manager.hpp"
#include "module2/preprocessor_db.hpp"
#include "db/db_config.hpp"
#include <iostream>

static void separator(const std::string& title) {
    std::cout << "\n======================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "======================================\n";
}

static int safeAddUser(SubmissionManager& mgr,
                       const std::string& name,
                       const std::string& email,
                       Role role) {
    try {
        int id = mgr.addUser(name, email, role);
        std::cout << "  Registered: " << name << " (id=" << id << ")\n";
        return id;
    } catch (...) {
        pqxx::connection conn(DB::CONNECTION_STRING);
        pqxx::work txn(conn);
        auto r = txn.exec_params(
            "SELECT userId FROM Users WHERE email=$1", email);
        txn.commit();
        int id = r[0][0].as<int>();
        std::cout << "  Already exists: " << name << " (id=" << id << ")\n";
        return id;
    }
}

int main() {
    std::cout << "\n======================================\n";
    std::cout << "       ContentGuard System\n";
    std::cout << "======================================\n";

    try {
        SubmissionManager mgr(DB::CONNECTION_STRING);
        PreprocessorDB    ppdb(DB::CONNECTION_STRING);
        std::cout << "\n✅ Connected to Neon PostgreSQL\n";

        // MODULE 1 — Register Users
        separator("MODULE 1 — Register Users");

        int uid1 = safeAddUser(mgr, "Aanya Godiyal",  "aanya@example.com",   Role::STUDENT);
        int uid2 = safeAddUser(mgr, "V Priya",         "priya@example.com",   Role::STUDENT);
        int uid3 = safeAddUser(mgr, "Drishti Painuli", "drishti@example.com", Role::STUDENT);
                   safeAddUser(mgr, "Yash Bahuguna",   "yash@example.com",    Role::STUDENT);
                   safeAddUser(mgr, "Prof. Sharma",    "sharma@example.com",  Role::INSTRUCTOR);

        std::cout << "\nAll users in database:\n";
        for (auto& u : mgr.listUsers(Role::STUDENT))
            std::cout << "  [Student]    id=" << u.userId << "  " << u.name << "\n";
        for (auto& u : mgr.listUsers(Role::INSTRUCTOR))
            std::cout << "  [Instructor] id=" << u.userId << "  " << u.name << "\n";

        // MODULE 1 — Submit Files
        separator("MODULE 1 — Add Submissions");

        std::string cppCode =
            "#include <iostream>\n"
            "int main() {\n"
            "    int studentScore = 0;  // initialize\n"
            "    studentScore = studentScore + 10;\n"
            "    /* compute final */\n"
            "    std::cout << studentScore;\n"
            "    return 0;\n"
            "}\n";

        std::string essay =
            "Artificial intelligence is transforming modern education. "
            "Students now have access to personalized learning tools that adapt "
            "to their individual pace and style. The impact of AI on assessment "
            "and plagiarism detection is particularly significant.";

        std::string cppCode2 =
            "#include <iostream>\n"
            "int main() {\n"
            "    int gradePoint = 0;  // init\n"
            "    gradePoint = gradePoint + 10;\n"
            "    /* calculate total */\n"
            "    std::cout << gradePoint;\n"
            "    return 0;\n"
            "}\n";

        int doc1 = mgr.addSubmission(uid1, "solution.cpp",  cppCode);
        int doc2 = mgr.addSubmission(uid2, "essay.txt",     essay);
        int doc3 = mgr.addSubmission(uid3, "solution2.cpp", cppCode2);

        std::cout << "Submissions saved:\n";
        std::cout << "  doc_id=" << doc1 << "  solution.cpp   (Aanya)   -> CODE\n";
        std::cout << "  doc_id=" << doc2 << "  essay.txt      (Priya)   -> TEXT\n";
        std::cout << "  doc_id=" << doc3 << "  solution2.cpp  (Drishti) -> CODE\n";

        mgr.updateStatus(doc1, SubmissionStatus::PROCESSING);
        mgr.updateStatus(doc2, SubmissionStatus::PROCESSING);
        mgr.updateStatus(doc3, SubmissionStatus::PROCESSING);
        std::cout << "\nAll submissions queued -> status=PROCESSING\n";

        // MODULE 2 — Preprocess
        separator("MODULE 2 — Preprocessing Pipeline");

        int processed = ppdb.processAllPending();
        std::cout << "\n✅ " << processed << " submission(s) preprocessed\n";

        separator("MODULE 2 — Preprocessed Results");

        auto results = ppdb.getAllResults();
        for (auto& doc : results) {
            std::cout << "\n  preprocessId=" << doc.preprocessId
                      << "  doc_id=" << doc.doc_id << "\n";
            std::cout << "  Tokens : " << doc.tokenCount << "\n";
            std::cout << "  Chars  : " << doc.charCount  << "\n";
            std::cout << "  Type   : "
                      << (doc.detectedType == FileType::CODE ? "CODE" : "TEXT") << "\n";
            std::string preview = doc.normalizedContent.substr(
                0, std::min((int)doc.normalizedContent.size(), 80));
            std::cout << "  Preview: " << preview << "...\n";
        }

        separator("Handoff -> Module 3 (Fingerprint Generation)");
        std::cout << "  PreprocessedDocs table has " << results.size() << " document(s)\n";
        std::cout << "  Module 3 reads normalizedContent + tokenCount using doc_id.\n";

        std::cout << "\n✅ Modules 1 & 2 complete.\n\n";

    } catch (const std::exception& e) {
        std::cerr << "\n Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
