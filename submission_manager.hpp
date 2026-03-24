#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <pqxx/pqxx>

enum class Role           { STUDENT, INSTRUCTOR };
enum class SubmissionType { TEXT, CODE };
enum class SubmissionStatus { PENDING, PROCESSING, DONE, ERROR };

struct User {
    int         userId;
    std::string name;
    std::string email;
    Role        role;
};

struct Submission {
    int              doc_id;          // ← renamed from submissionId
    int              userId;
    std::string      fileName;
    std::string      content;
    std::string      submittedAt;
    SubmissionType   type;
    SubmissionStatus status;
    int              fileSizeBytes;
};

class SubmissionManager {
public:
    explicit SubmissionManager(const std::string& connString);

    // User management
    int               addUser(const std::string& name,
                               const std::string& email,
                               Role role);
    User              getUser(int userId);
    bool              userExists(int userId);
    std::vector<User> listUsers(Role role);

    // Submission management — doc_id instead of submissionId
    int                     addSubmission(int userId,
                                          const std::string& fileName,
                                          const std::string& content);
    Submission              getSubmission(int doc_id);
    std::vector<Submission> getUserSubmissions(int userId);
    std::vector<Submission> getAllSubmissions();
    void                    updateStatus(int doc_id, SubmissionStatus newStatus);

    // Validation
    static SubmissionType detectType(const std::string& fileName);
    static bool           validateFile(const std::string& fileName,
                                       const std::string& content,
                                       std::string& outError);

    static std::string      roleToStr(Role r);
    static Role             strToRole(const std::string& s);
    static std::string      statusToStr(SubmissionStatus s);
    static SubmissionStatus strToStatus(const std::string& s);
    static std::string      typeToStr(SubmissionType t);
    static SubmissionType   strToType(const std::string& s);

private:
    pqxx::connection conn_;
};
