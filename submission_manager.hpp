#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <pqxx/pqxx>

// ─────────────────────────────────────────────────────────────
// Enums
// ─────────────────────────────────────────────────────────────
enum class Role           { STUDENT, INSTRUCTOR };
enum class SubmissionType { TEXT, CODE };
enum class SubmissionStatus { PENDING, PROCESSING, DONE, FAILED };

// ─────────────────────────────────────────────────────────────
// User — stores one registered user
// ─────────────────────────────────────────────────────────────
struct User {
    int         userId;
    std::string name;
    std::string email;
    Role        role;
};

// ─────────────────────────────────────────────────────────────
// Submission — stores one uploaded file
// ─────────────────────────────────────────────────────────────
struct Submission {
    int              doc_id;         // primary key — auto assigned by PostgreSQL SERIAL
    int              userId;         // foreign key → Users table
    std::string      fileName;
    std::string      content;        // raw file content
    std::string      submittedAt;    // timestamp
    SubmissionType   type;           // TEXT or CODE
    SubmissionStatus status;         // PENDING → PROCESSING → DONE / FAILED
    int              fileSizeBytes;
};

// ─────────────────────────────────────────────────────────────
// SubmissionManager — Module 1
// Handles all user and submission operations with Neon PostgreSQL
// ─────────────────────────────────────────────────────────────
class SubmissionManager {
public:
    explicit SubmissionManager(const std::string& connString);

    // ── User Operations ───────────────────────────────────────
    /** Register a new user. Returns auto-generated userId. */
    int               addUser(const std::string& name,
                               const std::string& email,
                               Role role);

    /** Fetch a user by userId. Throws if not found. */
    User              getUser(int userId);

    /** Fetch a user by email. Throws if not found. */
    User              getUserByEmail(const std::string& email);

    /** Returns true if userId exists in database. */
    bool              userExists(int userId);

    /** Returns all users with the given role. */
    std::vector<User> listUsers(Role role);

    /** Returns all users in database. */
    std::vector<User> listAllUsers();

    // ── Submission Operations ─────────────────────────────────
    /** Submit a file. Returns auto-generated doc_id. */
    int                     addSubmission(int userId,
                                          const std::string& fileName,
                                          const std::string& content);

    /** Fetch a submission by doc_id. Throws if not found. */
    Submission              getSubmission(int doc_id);

    /** Returns true if doc_id exists in database. */
    bool                    submissionExists(int doc_id);

    /** Returns all submissions by a specific user. */
    std::vector<Submission> getUserSubmissions(int userId);

    /** Returns all submissions in the system (instructor view). */
    std::vector<Submission> getAllSubmissions();

    /** Returns all submissions with a specific status. */
    std::vector<Submission> getSubmissionsByStatus(SubmissionStatus status);

    /** Update the status of a submission. */
    void                    updateStatus(int doc_id, SubmissionStatus newStatus);

    /** Delete a submission by doc_id. */
    void                    deleteSubmission(int doc_id);

    // ── Validation Helpers ────────────────────────────────────
    /** Detect file type from extension. */
    static SubmissionType detectType(const std::string& fileName);

    /**
     * Validate a file before accepting it.
     * Returns true if valid. Sets outError if invalid.
     * Checks: empty name, missing extension, empty content,
     *         file too large (>300,000 bytes), binary files.
     */
    static bool validateFile(const std::string& fileName,
                              const std::string& content,
                              std::string& outError);

    // ── String Converters (public — used by Module 2) ─────────
    static std::string      roleToStr(Role r);
    static Role             strToRole(const std::string& s);
    static std::string      statusToStr(SubmissionStatus s);
    static SubmissionStatus strToStatus(const std::string& s);
    static std::string      typeToStr(SubmissionType t);
    static SubmissionType   strToType(const std::string& s);

private:
    pqxx::connection conn_;
};
