#include <iostream>
#include "Exact_Match.h"
using namespace std;
void computeLPS(string pat, vector<int> &lps) {
    int len = 0;
    lps[0] = 0;
    int i = 1;
    while (i < pat.size()) {
        if (pat[i] == pat[len]) {
            len++;
            lps[i] = len;  
            i++;
        } else {
            if (len != 0) {
                len = lps[len - 1];
            } else {
                lps[i] = 0;
                i++;
            }
        }
    }
}
void kmpSearchAndInsert(string doc1, string doc2, string pattern, PGconn *conn) {
    vector<int> lps(pattern.size());
    computeLPS(pattern, lps);
    int i = 0, j = 0;
    while (i < doc1.size()) {
        if (doc1[i] == pattern[j]) {
            i++;
            j++;
        }
        if (j == pattern.size()) {
            int position = i - j;
            cout << "Match found at position: " << position << endl;
            int matchLength = pattern.size();
            float similarity = (float)matchLength / doc1.size();
            //insert query
            string query =
                "INSERT INTO matches (doc1, doc2, match_text, position, match_length, similarity, created_at) VALUES ('" +
                doc1 + "','" +
                doc2 + "','" +
                pattern + "'," +
                to_string(position) + "," +
                to_string(matchLength) + "," +
                to_string(similarity) + ", NOW());";
            PGresult *res = PQexec(conn, query.c_str());
            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                cout << " Insert failed: " << PQerrorMessage(conn) << endl;
            } else {
                cout << " Data inserted into DB!\n";
            }
            PQclear(res);
            j = lps[j - 1];
        }
        else if (i < doc1.size() && doc1[i] != pattern[j]) {
            if (j != 0)
                j = lps[j - 1];
            else
                i++;
        }
    }
}
