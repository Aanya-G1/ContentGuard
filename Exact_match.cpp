#include "httplib.h"
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

// Use nlohmann::json
using json = nlohmann::json;

// Simple KMP-like search (for demo, using std::string::find)
bool kmpSearch(const std::string &text, const std::string &pattern) {
    return text.find(pattern) != std::string::npos;
}

int main() {
    httplib::Server svr;

    svr.Post("/check", [&](const httplib::Request &req, httplib::Response &res) {
        // Parse JSON request
        json jsonReq = json::parse(req.body);
        std::string text = jsonReq["text"];
        std::string pattern = jsonReq["pattern"];

        // Run search
        bool found = kmpSearch(text, pattern);
        std::string result = found ? "Pattern found!" : "Pattern not found.";

        // Build JSON response
        json response;
        response["result"] = result;
        response["found"] = found;

        // Dummy matches list (since DB not connected yet)
        response["matches"] = {
            { {"id", 1}, {"text", "example one"} },
            { {"id", 2}, {"text", "example two"} }
        };

        // Send response
        res.set_content(response.dump(), "application/json");
    });

    std::cout << "Server running at http://localhost:5000\n";
    svr.listen("0.0.0.0", 5000);
}
