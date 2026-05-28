#ifndef DATABASEMANAGER_HPP
#define DATABASEMANAGER_HPP
#include <pqxx/pqxx>
#include <memory>
class DatabaseManager {
private:
    unique_ptr<pqxx::connection> connection;
public:
    DatabaseManager(
        const string& host,
        const string& db,
        const string& user,
        const string& password
    ) {
        string connStr =
            "host=" + host +
            " dbname=" + db +
            " user=" + user +
            " password=" + password;
        connection = make_unique<pqxx::connection>(connStr);
    }
    pqxx::connection& getConnection() {
        return *connection;
    }
};
#endif
