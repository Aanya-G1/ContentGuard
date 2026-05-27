#include "Fingerprint_module.h"
#include <iostream>

using namespace std;

FingerPrintGeneration::FingerPrintGeneration(int k_val) {
    this->k = k_val;
    h = 1;
    for (int i = 0; i < k - 1; i++) {
        h = (h * base) % mod;
    }
}

uint64_t FingerPrintGeneration::tokenToInt(const string& token) {
    return hash<string>{}(token);
}

vector<vector<string>> FingerPrintGeneration::kgramGeneration(vector<string>& tokens) {
    vector<vector<string>> kgrams;
    if (tokens.size() < k) return kgrams;
    for (size_t i = 0; i <= tokens.size() - k; i++) {
        vector<string> gram(tokens.begin() + i, tokens.begin() + i + k);
        kgrams.push_back(gram);
    }
    return kgrams;
}

void FingerPrintGeneration::generateHashes(vector<vector<string>> kgrams) {
    if (kgrams.empty()) return;
    hashes.clear();
    uint64_t hashValue = 0;

    // Hash for 1st kgram
    for (const auto& token : kgrams[0]) {
        hashValue = (hashValue * base + tokenToInt(token)) % mod;
    }
    hashes.push_back(hashValue);

    // Hash for other grams using rolling hash 
    for (size_t i = 1; i < kgrams.size(); i++) {
        hashValue = rollinghashGeneration(hashValue, tokenToInt(kgrams[i - 1][0]), tokenToInt(kgrams[i][k - 1]));
        hashes.push_back(hashValue);
    }
}
uint64_t FingerPrintGeneration::rollinghashGeneration(uint64_t prevHash,uint64_t OutVal,uint64_t InVal){
    uint64_t nextHash=(base*(prevHash-OutVal*h)+InVal) % mod;
    return nextHash;
}

vector<uint64_t> FingerPrintGeneration::winnowing(const vector<uint64_t>& hashes){
    int w=4;
    deque<int> dq;
    vector<uint64_t> selectedHashes;
    for(int i=0;i<hashes.size();i++){
        if(!dq.empty() && dq.front()<= i - w) dq.pop_front();
        while(!dq.empty() && hashes[dq.back()]>hashes[i]) dq.pop_back();
        dq.push_back(i);
       if (i >= w - 1) {
            uint64_t currentMin = hashes[dq.front()];

            if (selectedHashes.empty() || currentMin != selectedHashes.back()) {
                selectedHashes.push_back(currentMin);
            }
        }
    }
    return selectedHashes;
}
vector<uint64_t> FingerPrintGeneration::getHashes() const {
    return hashes;
}

void DatabaseOperations::Connection(){
    try{
        string url = 
    "postgresql://neondb_owner:npg_2TJFqNlzPp5Y@"
    "ep-calm-heart-a1fxv4n4-pooler.ap-southeast-1.aws.neon.tech/"
    "contentGuard?sslmode=require&channel_binding=require";
        conn=make_unique<pqxx::connection>(url);
    }
    catch(const exception &e){
        cout<<"Connection to database failed! "<<e.what()<<endl;
    }
}
        
void DatabaseOperations::storeFingerprints(int docId, const vector<uint64_t>& hashes){
    try{
        pqxx::work txn(*conn);
        txn.exec("CREATE TABLE IF NOT EXISTS fingerprints (id SERIAL PRIMARY KEY,doc_id INT,hash_value BIGINT);");
        for(auto hash: hashes){
            txn.exec_params("INSERT INTO fingerprints(doc_id,hash_value) VALUES($1,$2)",docId,hash);
        }
        txn.commit();
        cout << "Stored " << hashes.size() << " fingerprints successfully.\n";
    }
    catch(const exception &e){
        cout<<"Error storing fingerprints in database! "<<e.what()<<endl;
    }
}
vector<uint64_t> DatabaseOperations::getFingerprints(int docId){
    vector<uint64_t> hashes;
    try{
        pqxx::read_transaction txn(*conn);
        for(auto [hash_val]:txn.query<uint64_t>("SELECT hash_value FROM fingerprints WHERE doc_id = $1", docId)) {
            hashes.push_back(hash_val);
        }
    }
    catch (const exception &e) {
        cout << "Couldn't fetch fingerprints! " << e.what() << endl;
    }      
    return hashes;
}
vector<int> DatabaseOperations::findMatch(const vector<uint64_t>& hashes){
    vector<int> docIds;
    if (hashes.empty()) return docIds;
    try{
        pqxx::read_transaction txn(*conn);
        string hashList;
        for (size_t i = 0; i < hashes.size(); ++i) {
            hashList += to_string(hashes[i]) + (i == hashes.size() - 1 ? "" : ",");
        }
        string query = "SELECT doc_id FROM fingerprints WHERE hash_value IN (" + hashList + 
                       ") GROUP BY doc_id HAVING COUNT(*) > 2";
        
        pqxx::result res = txn.exec(query);
        for (auto row : res) {
            docIds.push_back(row[0].as<int>());
        }
    }
    catch (const exception &e) {
        cout << "Couldn't Perform filtering! " << e.what() << endl;
    }  
    return docIds;
}
vector<int> fingerprintProcessing(int docId,vector<std::string> tokens){
    DatabaseOperations db;
    db.Connection(); 
    FingerPrintGeneration fg(3);
    fg.generateHashes(fg.kgramGeneration(tokens));
    vector<int> matches = db.findMatch(fg.getHashes());
    db.storeFingerprints(docId, fg.getHashes());
    return matches;
}