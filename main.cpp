/**
 * ╔══════════════════════════════════════════════════════════╗
 * ║           ContentGuard — HTTP API Server                 ║
 * ║  Connects C++ backend to the HTML/JS frontend            ║
 * ╚══════════════════════════════════════════════════════════╝
 *
 * Dependencies (header-only, place in project root):
 *   httplib.h  — https://github.com/yhirose/cpp-httplib/releases  (single header)
 *   json.hpp   — https://github.com/nlohmann/json/releases        (single header)
 *
 * Build:
 *   g++ -std=c++17 main.cpp module1/submission_manager.cpp \
 *       module2/preprocessor.cpp module2/preprocessor_db.cpp \
 *       -I. -lpqxx -lpq -lssl -lcrypto -lpthread -o contentguard
 *
 * Run:
 *   ./contentguard
 *   Server starts at http://localhost:8080
 */

#include "httplib.h"          // cpp-httplib  (place in project root)
#include "json.hpp"           // nlohmann/json (place in project root)
#include "submission_manager.hpp"
#include "preprocessor_db.hpp"
#include "db/db_config.hpp"

#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <string>

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────
// Utility helpers
// ─────────────────────────────────────────────────────────────

/** Returns "YYYY-MM-DD HH:MM" in local time */
static std::string currentTimestamp() {
    std::time_t t  = std::time(nullptr);
    std::tm*    tm = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

/** Returns a unique batch ID string */
static std::string makeBatchId() {
    std::ostringstream oss;
    oss << "BATCH-" << std::hex << std::uppercase << std::time(nullptr);
    return oss.str();
}

/** Returns "SUB-<docId>" */
static std::string makeSubId(int docId) {
    return "SUB-" + std::to_string(docId);
}

/** Returns an emoji for the file extension (matches frontend logic) */
static std::string getEmoji(const std::string& fileName) {
    auto dot = fileName.rfind('.');
    if (dot == std::string::npos) return "📄";
    std::string ext = fileName.substr(dot + 1);
    if (ext == "java") return "☕";
    if (ext == "py")   return "🐍";
    if (ext == "cpp" || ext == "c") return "⚙️";
    if (ext == "js")   return "🟨";
    if (ext == "html") return "🌐";
    if (ext == "pdf")  return "📕";
    if (ext == "cs")   return "🔷";
    return "📄";
}

/** Formats byte count as human-readable string */
static std::string formatSize(int bytes) {
    if (bytes < 1024)    return std::to_string(bytes) + " B";
    if (bytes < 1048576) return std::to_string(bytes / 1024) + " KB";
    return std::to_string(bytes / 1048576) + " MB";
}

/** Maps a plagiarism score (0–100) to a risk label */
static std::string scoreToLabel(double score) {
    if (score >= 70) return "HIGH";
    if (score >= 40) return "MEDIUM";
    if (score >= 10) return "LOW";
    return "NONE";
}

// ─────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────
int main() {
    std::cout << "\n╔══════════════════════════════════════╗\n";
    std::cout <<   "║   ContentGuard API Server             ║\n";
    std::cout <<   "╚══════════════════════════════════════╝\n\n";

    // Connect to Neon PostgreSQL
    SubmissionManager mgr(DB::CONNECTION_STRING);
    PreprocessorDB    ppdb(DB::CONNECTION_STRING);
    std::cout << "✅ Connected to Neon PostgreSQL\n\n";

    httplib::Server svr;

    // ── CORS headers (required so browser can call this API) ──
    svr.set_default_headers({
        {"Access-Control-Allow-Origin",  "*"},
        {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Accept"}
    });

    // Handle browser preflight OPTIONS requests
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
        res.set_content("", "text/plain");
    });

    // ══════════════════════════════════════════════════════════
    // POST /api/submit
    //
    // Called by upload.html when user clicks "Start Scan".
    //
    // Request body (JSON):
    //   {
    //     "userId":         "STU-2024-001",   // from the Student/User ID field
    //     "fileName":       "solution.cpp",   // uploaded file name
    //     "content":        "...",            // file text content
    //     "submissionName": "Assignment 3"    // from the Submission Name field
    //   }
    //
    // Response (JSON):
    //   {
    //     "success":   true,
    //     "doc_id":    5,
    //     "subId":     "SUB-5",
    //     "batchId":   "BATCH-ABC123",
    //     "score":     0.0,
    //     "label":     "NONE",
    //     "submitted": "2026-05-28 12:00",
    //     "fileName":  "solution.cpp",
    //     "subName":   "Assignment 3",
    //     "user":      "STU-2024-001",
    //     "size":      "1.2 KB",
    //     "icon":      "⚙️"
    //   }
    // ══════════════════════════════════════════════════════════
    svr.Post("/api/submit", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        try {
            auto body = json::parse(req.body);

            std::string userId   = body.value("userId",         "UNKNOWN");
            std::string fileName = body.value("fileName",       "file.txt");
            std::string content  = body.value("content",        "");
            std::string subName  = body.value("submissionName", fileName);

            // ── Validate file ──────────────────────────────────
            std::string err;
            if (!SubmissionManager::validateFile(fileName, content, err)) {
                res.status = 400;
                res.set_content(json{{"success", false}, {"error", err}}.dump(),
                                "application/json");
                return;
            }

            // ── Get or create user ─────────────────────────────
            // We store the frontend's userId string as the user's name
            // and use userId@contentguard.local as a synthetic email.
            int dbUserId = -1;
            std::string userEmail = userId + "@contentguard.local";
            try {
                User u = mgr.getUserByEmail(userEmail);
                dbUserId = u.userId;
            } catch (...) {
                // First time this userId appears — register them
                dbUserId = mgr.addUser(userId, userEmail, Role::STUDENT);
            }

            // ── Save submission (Module 1) ─────────────────────
            int docId = mgr.addSubmission(dbUserId, fileName, content);
            mgr.updateStatus(docId, SubmissionStatus::PROCESSING);

            // ── Preprocess (Module 2) ──────────────────────────
            try {
                ppdb.processSubmission(docId);
                mgr.updateStatus(docId, SubmissionStatus::DONE);
            } catch (const std::exception& ppErr) {
                std::cerr << "Preprocessor error for doc_id=" << docId
                          << ": " << ppErr.what() << "\n";
                mgr.updateStatus(docId, SubmissionStatus::ERROR);
            }

            // ── Score (Module 3 / 4 placeholder) ──────────────
            // TODO: When Module 3 (fingerprinting) and Module 4 (comparison)
            // are implemented, replace 0.0 below with the real similarity score:
            //
            //   FingerprintEngine fp(DB::CONNECTION_STRING);
            //   double score = fp.getSimilarityScore(docId);
            //
            double score = 0.0;
            std::string label = scoreToLabel(score);

            // ── Build and return response ──────────────────────
            json response = {
                {"success",   true},
                {"doc_id",    docId},
                {"subId",     makeSubId(docId)},
                {"batchId",   makeBatchId()},
                {"score",     score},
                {"label",     label},
                {"submitted", currentTimestamp()},
                {"fileName",  fileName},
                {"subName",   subName},
                {"user",      userId},
                {"size",      formatSize((int)content.size())},
                {"icon",      getEmoji(fileName)}
            };
            res.set_content(response.dump(), "application/json");

        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                json{{"success", false}, {"error", e.what()}}.dump(),
                "application/json");
        }
    });

    // ══════════════════════════════════════════════════════════
    // GET /api/documents
    //
    // Called by documents.html to load all submissions.
    //
    // Response (JSON array):
    //   [
    //     {
    //       "id":        "SUB-5",
    //       "doc_id":    5,
    //       "batchId":   "BATCH-5",
    //       "name":      "solution.cpp",
    //       "subName":   "solution.cpp",
    //       "user":      "STU-2024-001",
    //       "size":      "1.2 KB",
    //       "submitted": "2026-05-28 12:00",
    //       "score":     0.0,
    //       "label":     "NONE",
    //       "status":    "PROCESSED",
    //       "rank":      null,
    //       "icon":      "⚙️"
    //     },
    //     ...
    //   ]
    // ══════════════════════════════════════════════════════════
    svr.Get("/api/documents", [&](const httplib::Request&, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        try {
            auto submissions = mgr.getAllSubmissions();
            json arr = json::array();

            for (auto& s : submissions) {
                // Recover the original userId string (stored as user's name)
                std::string userStr = "UNKNOWN";
                try {
                    User u = mgr.getUser(s.userId);
                    userStr = u.name;   // name = original frontend userId string
                } catch (...) {}

                // Map submission status → frontend status string
                std::string docStatus =
                    (s.status == SubmissionStatus::DONE) ? "PROCESSED" : "PENDING";

                // Score placeholder — replace with Module 3/4 call
                double score = 0.0;
                // TODO: score = fingerprintEngine.getScore(s.doc_id);

                std::string label = scoreToLabel(score);

                arr.push_back({
                    {"id",        makeSubId(s.doc_id)},
                    {"doc_id",    s.doc_id},
                    {"batchId",   "BATCH-" + std::to_string(s.doc_id)},
                    {"name",      s.fileName},
                    {"subName",   s.fileName},
                    {"user",      userStr},
                    {"size",      formatSize(s.fileSizeBytes)},
                    {"submitted", s.submittedAt},
                    {"score",     score},
                    {"label",     label},
                    {"status",    docStatus},
                    {"rank",      nullptr},   // ranked client-side
                    {"icon",      getEmoji(s.fileName)}
                });
            }

            res.set_content(arr.dump(), "application/json");

        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                json{{"error", e.what()}}.dump(),
                "application/json");
        }
    });

    // ══════════════════════════════════════════════════════════
    // DELETE /api/documents
    //
    // Called by documents.html "Clear All" button.
    // Removes all submissions and their preprocessed results.
    // ══════════════════════════════════════════════════════════
    svr.Delete("/api/documents", [&](const httplib::Request&, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        try {
            auto submissions = mgr.getAllSubmissions();
            for (auto& s : submissions) {
                try { ppdb.deleteResult(s.doc_id); } catch (...) {}
                mgr.deleteSubmission(s.doc_id);
            }
            res.set_content(json{{"success", true}}.dump(), "application/json");

        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                json{{"success", false}, {"error", e.what()}}.dump(),
                "application/json");
        }
    });

    // ══════════════════════════════════════════════════════════
    // GET /api/health   (optional — useful for frontend to check if server is up)
    // ══════════════════════════════════════════════════════════
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(
            json{{"status", "ok"}, {"service", "ContentGuard"}}.dump(),
            "application/json");
    });

    // ── Start listening ────────────────────────────────────────
    std::cout << "ContentGuard API running on http://localhost:8080\n\n";
    std::cout << "Endpoints:\n";
    std::cout << "  POST   /api/submit        — upload & process a document\n";
    std::cout << "  GET    /api/documents     — list all documents\n";
    std::cout << "  DELETE /api/documents     — clear all documents\n";
    std::cout << "  GET    /api/health        — health check\n\n";
    std::cout << "Open the frontend: file:///path/to/index.html\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    svr.listen("0.0.0.0", 8080);
    return 0;
}
