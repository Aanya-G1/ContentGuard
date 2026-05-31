/**
 * ContentGuard — HTTP API Server (production entry point)
 * Build: make contentguard
 * Run:   ./contentguard  →  http://localhost:8080
 */

#include "httplib.h"
#include "json.hpp"
#include "submission_manager.hpp"
#include "preprocessor/preprocessor_db.hpp"
#include "database/db_config.hpp"
#include "pipeline/pipeline.hpp"
#include "reporting/reporting.hpp"
#include <pqxx/pqxx>

#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <string>

using json = nlohmann::json;

static std::string currentTimestamp() {
    std::time_t t  = std::time(nullptr);
    std::tm*    tm = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

static std::string makeSubId(int docId) {
    return "SUB-" + std::to_string(docId);
}

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

static std::string formatSize(int bytes) {
    if (bytes < 1024)    return std::to_string(bytes) + " B";
    if (bytes < 1048576) return std::to_string(bytes / 1024) + " KB";
    return std::to_string(bytes / 1048576) + " MB";
}

int main() {
    std::cout << "\n╔══════════════════════════════════════╗\n";
    std::cout <<   "║   ContentGuard API Server             ║\n";
    std::cout <<   "╚══════════════════════════════════════╝\n\n";

    SubmissionManager mgr(DB::CONNECTION_STRING);
    PreprocessorDB    ppdb(DB::CONNECTION_STRING);
    ContentGuardPipeline pipeline(DB::CONNECTION_STRING, mgr, ppdb);
    pipeline.hydrateFromDatabase();

    std::cout << "Connected to PostgreSQL\n";
    std::cout << "Loaded persisted scan reports from database\n\n";

    httplib::Server svr;

    svr.set_default_headers({
        {"Access-Control-Allow-Origin",  "*"},
        {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type, Accept"}
    });

    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
        res.set_content("", "text/plain");
    });

    svr.Post("/api/submit", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        try {
            auto body = json::parse(req.body);

            std::string userId   = body.value("userId",         "UNKNOWN");
            std::string fileName = body.value("fileName",       "file.txt");
            std::string content  = body.value("content",        "");
            std::string subName  = body.value("submissionName", fileName);

            std::string err;
            if (!SubmissionManager::validateFile(fileName, content, err)) {
                res.status = 400;
                res.set_content(json{{"success", false}, {"error", err}}.dump(),
                                "application/json");
                return;
            }

            int dbUserId = -1;
            std::string userEmail = userId + "@contentguard.local";
            try {
                User u = mgr.getUserByEmail(userEmail);
                dbUserId = u.userId;
            } catch (...) {
                dbUserId = mgr.addUser(userId, userEmail, Role::STUDENT);
            }

            int docId = mgr.addSubmission(dbUserId, fileName, content);
            mgr.updateStatus(docId, SubmissionStatus::PROCESSING);

            try {
                ppdb.processSubmission(docId);
                mgr.updateStatus(docId, SubmissionStatus::DONE);
            } catch (const std::exception& ppErr) {
                std::cerr << "Preprocessor error doc_id=" << docId << ": "
                          << ppErr.what() << "\n";
                mgr.updateStatus(docId, SubmissionStatus::FAILED);
                res.status = 500;
                res.set_content(
                    json{{"success", false}, {"error", ppErr.what()}}.dump(),
                    "application/json");
                return;
            }

            ScanReport report = pipeline.analyzeSubmission(docId, content);

            json meta = {
                {"doc_id",    docId},
                {"subId",     makeSubId(docId)},
                {"submitted", currentTimestamp()},
                {"fileName",  fileName},
                {"subName",   subName},
                {"user",      userId},
                {"size",      formatSize((int)content.size())},
                {"icon",      getEmoji(fileName)}
            };
            json response = ReportingModule::buildSubmitResponse(report, meta);
            res.set_content(response.dump(), "application/json");

        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                json{{"success", false}, {"error", e.what()}}.dump(),
                "application/json");
        }
    });

    svr.Get(R"(/api/report/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        try {
            int docId = std::stoi(req.matches[1]);
            ScanReport report = pipeline.getReport(docId);
            if (report.docId == 0) {
                res.status = 404;
                res.set_content(json{{"error", "Report not found"}}.dump(),
                                "application/json");
                return;
            }
            json out = pipeline.reportToJson(report);
            out["success"] = true;
            res.set_content(out.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    svr.Get("/api/documents", [&](const httplib::Request&, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        try {
            auto submissions = mgr.getAllSubmissions();
            json arr = json::array();

            for (auto& s : submissions) {
                std::string userStr = "UNKNOWN";
                try {
                    User u = mgr.getUser(s.userId);
                    userStr = u.name;
                } catch (...) {}

                std::string docStatus =
                    (s.status == SubmissionStatus::DONE) ? "PROCESSED" : "PENDING";

                ScanReport report = pipeline.getReport(s.doc_id);
                double score = report.docId ? report.score : 0.0;
                std::string label = report.docId ? report.label : "NONE";
                int rank = report.docId ? report.rank : 0;

                arr.push_back({
                    {"id",             makeSubId(s.doc_id)},
                    {"doc_id",         s.doc_id},
                    {"batchId",        report.batchId.empty()
                                           ? "BATCH-" + std::to_string(s.doc_id)
                                           : report.batchId},
                    {"name",           s.fileName},
                    {"subName",        s.fileName},
                    {"user",           userStr},
                    {"size",           formatSize(s.fileSizeBytes)},
                    {"submitted",      s.submittedAt},
                    {"score",          score},
                    {"label",          label},
                    {"classification", report.classification.empty() ? "NONE" : report.classification},
                    {"status",         docStatus},
                    {"rank",           rank > 0 ? json(rank) : json(nullptr)},
                    {"icon",           getEmoji(s.fileName)}
                });
            }

            res.set_content(arr.dump(), "application/json");

        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    svr.Delete("/api/documents", [&](const httplib::Request&, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        try {
            auto submissions = mgr.getAllSubmissions();

            for (auto& s : submissions) {
                try { ppdb.deleteResult(s.doc_id); } catch (...) {}
                try {
                    pqxx::connection c(DB::CONNECTION_STRING);
                    pqxx::work w(c);
                    w.exec("DELETE FROM fingerprints WHERE doc_id=$1", pqxx::params{s.doc_id});
                    w.exec("DELETE FROM scan_results WHERE doc_id=$1", pqxx::params{s.doc_id});
                    w.commit();
                } catch (...) {}
                mgr.deleteSubmission(s.doc_id);
            }
            pipeline.clearAllReports();
            res.set_content(json{{"success", true}}.dump(), "application/json");

        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(
                json{{"success", false}, {"error", e.what()}}.dump(),
                "application/json");
        }
    });

    svr.Get("/api/ranked", [&](const httplib::Request&, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        json out;
        out["success"] = true;
        out["ranked"]  = pipeline.rankedListJson();
        res.set_content(out.dump(), "application/json");
    });

    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(
            json{{"status", "ok"}, {"service", "ContentGuard"}}.dump(),
            "application/json");
    });

    std::cout << "ContentGuard API → http://localhost:8080\n";
    std::cout << "  POST   /api/submit\n";
    std::cout << "  GET    /api/documents\n";
    std::cout << "  GET    /api/report/{doc_id}\n";
    std::cout << "  GET    /api/ranked\n";
    std::cout << "  DELETE /api/documents\n";
    std::cout << "  GET    /api/health\n\n";

    svr.listen("0.0.0.0", 8080);
    return 0;
}
