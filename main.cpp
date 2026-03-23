//testing
#include<iostream>
#include "Fingerprint_module.h"
using namespace std;
int main() {
    DatabaseOperations db;
    db.Connection(); 
    cout << "Testing Fingerprint Generation" << endl;
    vector<string> original = {"int", "main", "{", "cout", "<<", "hello", ";", "}"};
    FingerPrintGeneration fg(3);
    fg.generateHashes(fg.kgramGeneration(original));
    db.storeFingerprints(101, fg.getHashes());

    vector<string> suspect = {"int", "main", "{", "cout", "<<", "hello", "<<", "endl", ";", "}"};
    fg.generateHashes(fg.kgramGeneration(suspect));
    vector<uint64_t> suspectHashes = fg.getHashes();

   cout << "\n--- Searching for matches for new submission ---\n";
    vector<int> matches = db.findMatch(suspectHashes);

    if (matches.empty()) {
        cout << "No matches found. This document is clean.\n";
    } else {
        cout << "ALERT: Potential plagiarism detected in Doc IDs: ";
        for (int id : matches) cout << id << " ";
        cout << endl;
    }

    return 0;
}