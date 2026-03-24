#ifndef Exact_Match_H
#define Exact_Match_H

#include <string>
#include <vector>
#include <libpq-fe.h>

void computeLPS(std::string pat, std::vector<int> &lps);
void kmpSearchAndInsert(std::string doc1, std::string doc2, std::string pattern, PGconn *conn);

#endif