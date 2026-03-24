#include "submission_manager.hpp"
#include <algorithm>
#include <cctype>
#include <unordered_set>

static const int MAX_FILE_BYTES = 300000;

static std::string toLower(const std::string& s) {
    std::string o = s;
    std::transform(o.begin(), o.end(), o.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return o;
}

static const std::unordered_set<std::string> CODE_EXTS = {
    ".cpp",".cc",".c",".h",".hpp",".java",".py",
    ".js",".ts",".cs",".rb",".go",".rs",".swift",".kt",".php"
};

std::string SubmissionManager::roleToStr(Role r) {
    return r == Role::STUDENT ? "STUDENT" : "INSTRUCTOR";
}
Role SubmissionManager::strToRole(const std::string& s) {
    return s == "STUDENT" ? Role::STUDENT : Role::INSTRUCTOR;
}
std::string SubmissionManager::statusToStr(SubmissionStatus s) {
    switch (s) {
        case SubmissionStatus::PROCESSING: return "PROCESSING";
        case SubmissionStatus::DONE:       return "DONE";
        case SubmissionStatus::ERROR:      return "ERROR";
        default:                           return "PENDING";
    }
}
SubmissionStatus SubmissionManager::strToStatus(const std::string& s) {
    if (s == "PROCESSING") return SubmissionStatus::PROCESSING;
    if (s == "DONE")       return SubmissionStatus::DONE;
    if (s == "ERROR")      return SubmissionStatus::ERROR;
    return SubmissionStatus::PENDING;
}
std::string SubmissionManager::typeToStr(SubmissionType t) {
    return t == SubmissionType::CODE ? "CODE" : "TEXT";
}
SubmissionType SubmissionManager::strToType(const std::string& s) {
    return s == "CODE" ? SubmissionType::CODE : SubmissionType::TEXT;
}

SubmissionManager::SubmissionManager(const std::string& connString)
    : conn_(connString) {}

SubmissionType SubmissionManager::detectType(const std::string& fileName) {
    auto dot = fileName.rfind('.');
    if (dot == std::string::npos) return SubmissionType::TEXT;
    return CODE_EXTS.count(toLower(fileName.substr(dot)))
           ? SubmissionType::CODE : SubmissionType::TEXT;
}

bool SubmissionManager::validateFile(const std::string& fileName,
                                     const std::string& content,
                                     std::string& outError) {
    if (fileName.empty())
        { outError = "File name cannot be empty."; return false; }
    if (fileName.rfind('.') == std::string::npos)
        { outError = "File must have an extension."; return false; }
    if (content.empty())
        { outError = "File content is empty."; return false; }
    if ((int)content.size() > MAX_FILE_BYTES)
        { outError = "File too large. Maximum ~3000 lines."; return false; }
    if (content.find('\0') != std::string::npos)
        { outError = "Binary files not supported."; return false; }
    outError.clear();
    return true;
}

int SubmissionManager::addUser(const std::string& name,
                                const std::string& email,
                                Role role) {
    if (name.empty() || email.empty())
        throw std::invalid_argument("Name and email must not be empty.");
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "INSERT INTO Users(name, email, role) VALUES($1,$2,$3) RETURNING userId",
        name, email, roleToStr(role));
    txn.commit();
    return r[0][0].as<int>();
}

User SubmissionManager::getUser(int userId) {
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT userId, name, email, role FROM Users WHERE userId=$1", userId);
    txn.commit();
    if (r.empty())
        throw std::runtime_error("User not found: " + std::to_string(userId));
    return { r[0][0].as<int>(), r[0][1].c_str(),
             r[0][2].c_str(), strToRole(r[0][3].c_str()) };
}

bool SubmissionManager::userExists(int userId) {
    pqxx::work txn(conn_);
    auto r = txn.exec_params("SELECT 1 FROM Users WHERE userId=$1", userId);
    txn.commit();
    return !r.empty();
}

std::vector<User> SubmissionManager::listUsers(Role role) {
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT userId, name, email, role FROM Users WHERE role=$1",
        roleToStr(role));
    txn.commit();
    std::vector<User> result;
    for (const auto& row : r)
        result.push_back({ row[0].as<int>(), row[1].c_str(),
                           row[2].c_str(), strToRole(row[3].c_str()) });
    return result;
}

int SubmissionManager::addSubmission(int userId,
                                      const std::string& fileName,
                                      const std::string& content) {
    if (!userExists(userId))
        throw std::runtime_error("User not found: " + std::to_string(userId));
    std::string err;
    if (!validateFile(fileName, content, err))
        throw std::invalid_argument("Validation failed: " + err);

    pqxx::work txn(conn_);
    // doc_id is SERIAL — auto assigned by PostgreSQL
    auto r = txn.exec_params(
        "INSERT INTO Submissions(userId, fileName, content, fileType, fileSizeBytes)"
        " VALUES($1,$2,$3,$4,$5) RETURNING doc_id",
        userId, fileName, content,
        typeToStr(detectType(fileName)),
        (int)content.size());
    txn.commit();
    return r[0][0].as<int>();
}

Submission SubmissionManager::getSubmission(int doc_id) {
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT doc_id, userId, fileName, content,"
        "       fileType, status, submittedAt, fileSizeBytes"
        " FROM Submissions WHERE doc_id=$1", doc_id);
    txn.commit();
    if (r.empty())
        throw std::runtime_error("Submission not found: " + std::to_string(doc_id));
    const auto& row = r[0];
    return { row[0].as<int>(), row[1].as<int>(),
             row[2].c_str(),   row[3].c_str(),
             row[6].c_str(),
             strToType(row[4].c_str()),
             strToStatus(row[5].c_str()),
             row[7].as<int>() };
}

std::vector<Submission> SubmissionManager::getUserSubmissions(int userId) {
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "SELECT doc_id, userId, fileName, content,"
        "       fileType, status, submittedAt, fileSizeBytes"
        " FROM Submissions WHERE userId=$1 ORDER BY submittedAt DESC", userId);
    txn.commit();
    std::vector<Submission> result;
    for (const auto& row : r)
        result.push_back({ row[0].as<int>(), row[1].as<int>(),
                           row[2].c_str(),   row[3].c_str(),
                           row[6].c_str(),
                           strToType(row[4].c_str()),
                           strToStatus(row[5].c_str()),
                           row[7].as<int>() });
    return result;
}

std::vector<Submission> SubmissionManager::getAllSubmissions() {
    pqxx::work txn(conn_);
    auto r = txn.exec(
        "SELECT doc_id, userId, fileName, content,"
        "       fileType, status, submittedAt, fileSizeBytes"
        " FROM Submissions ORDER BY submittedAt DESC");
    txn.commit();
    std::vector<Submission> result;
    for (const auto& row : r)
        result.push_back({ row[0].as<int>(), row[1].as<int>(),
                           row[2].c_str(),   row[3].c_str(),
                           row[6].c_str(),
                           strToType(row[4].c_str()),
                           strToStatus(row[5].c_str()),
                           row[7].as<int>() });
    return result;
}

void SubmissionManager::updateStatus(int doc_id, SubmissionStatus newStatus) {
    pqxx::work txn(conn_);
    auto r = txn.exec_params(
        "UPDATE Submissions SET status=$1 WHERE doc_id=$2 RETURNING doc_id",
        statusToStr(newStatus), doc_id);
    txn.commit();
    if (r.empty())
        throw std::runtime_error("Submission not found: " + std::to_string(doc_id));
}
