#include<iostream>
#include "Exact_Match.h"
using namespace std;
int main() {
    string doc1 = "this is plagiarism detection system";
    string doc2 = "plagiarism detection is important";
    string pattern = "plagiarism";
    PGconn *conn = PQconnectdb(
        "postgresql://neondb_owner:npg_2TJFqNlzPp5Y@ep-calm-heart-a1fxv4n4-pooler.ap-southeast-1.aws.neon.tech/contentGuard?sslmode=require"
    );
    if (PQstatus(conn) != CONNECTION_OK) {
        cout << " Connection failed!\n";
        cout << PQerrorMessage(conn);
        PQfinish(conn);
        return 1;
    }
    cout << " Connected to DB!\n";
    kmpSearchAndInsert(doc1, doc2, pattern, conn);
    PQfinish(conn);
    return 0;
}