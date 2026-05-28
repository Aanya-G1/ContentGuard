#include "httplib.h"
#include "json.hpp"

#include <vector>
#include <string>
#include <iostream>

using json = nlohmann::json;

// ---------------- KMP Algorithm ----------------

// Build LPS array
std::vector<int> buildLPS(const std::string &pattern) {
    int m = pattern.size();
    std::vector<int> lps(m, 0);

    int len = 0;
    int i = 1;

    while (i < m) {
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        }
        else {
            if (len != 0) {
                len = lps[len - 1];
            }
            else {
                lps[i] = 0;
                i++;
            }
        }
    }

    return lps;
}

// KMP Search
bool kmpSearch(const std::string &text, const std::string &pattern) {

    if (pattern.empty())
        return false;

    int n = text.size();
    int m = pattern.size();

    std::vector<int> lps = buildLPS(pattern);

    int i = 0; // text index
    int j = 0; // pattern index

    while (i < n) {

        if (text[i] == pattern[j]) {
            i++;
            j++;
        }

        if (j == m) {
            return true;
        }
        else if (i < n && text[i] != pattern[j]) {

            if (j != 0)
                j = lps[j - 1];
            else
                i++;
        }
    }

    return false;
}

// ---------------- Enable CORS ----------------

void enable_cors(httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// ---------------- Main Server ----------------

int main() {

    httplib::Server svr;

    // Handle OPTIONS request (CORS preflight)
    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        enable_cors(res);
    });

    // Health check API
    svr.Get("/api", [](const httplib::Request&, httplib::Response& res) {

        enable_cors(res);

        json response;
        response["message"] = "Backend connected ✅";

        res.set_content(response.dump(), "application/json");
    });

    // Pattern checking API
    svr.Post("/check", [](const httplib::Request& req, httplib::Response& res) {

        enable_cors(res);

        try {

            auto body = json::parse(req.body);

            std::string text = body["text"];
            std::string pattern = body["pattern"];

            bool found = kmpSearch(text, pattern);

            json result;

            result["found"] = found;

            if (found)
                result["result"] = "Pattern found in text!";
            else
                result["result"] = "Pattern not found.";

            res.set_content(result.dump(), "application/json");
        }
        catch (...) {

            json error;
            error["error"] = "Invalid JSON data";

            res.status = 400;

            res.set_content(error.dump(), "application/json");
        }
    });

    std::cout << "Server started at http://localhost:5000\n";

    svr.listen("0.0.0.0", 5000);

    return 0;
}
