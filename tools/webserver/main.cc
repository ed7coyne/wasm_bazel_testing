#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <map>
#include <uwebsockets/App.h>
#include "tools/cpp/runfiles/runfiles.h"

namespace fs = std::filesystem;

// Global runfiles object
rules_cc::cc::runfiles::Runfiles* runfiles = nullptr;

// MIME type mapping
std::map<std::string, std::string> mimeTypes = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".txt", "text/plain"},
    {".wasm", "application/wasm"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".ttf", "font/ttf"},
    {".eot", "application/vnd.ms-fontobject"},
};

std::string getMimeType(const std::string& path) {
    for (const auto& [ext, mime] : mimeTypes) {
        if (path.length() >= ext.length() && 
            path.compare(path.length() - ext.length(), ext.length(), ext) == 0) {
            return mime;
        }
    }
    return "application/octet-stream";
}

std::string getRunfilesPath(const std::string& relativePath) {
    // We require runfiles to be initialized
    if (runfiles == nullptr) {
        return "";
    }
    
    // Remove leading slash if present
    std::string path = relativePath;
    if (!path.empty() && path[0] == '/') {
        path = path.substr(1);
    }
    
    // Use the workspace name as prefix for runfiles paths
    std::string fullPath = "wasm-bazel/" + path;
    
    // Use Rlocation to find the file in runfiles
    std::string resolvedPath = runfiles->Rlocation(fullPath);
    
    if (resolvedPath.empty()) {
        std::cerr << "Warning: Could not resolve path in runfiles: " << fullPath << std::endl;
        return "";
    }
    
    return resolvedPath;
}

std::string readFileContent(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    return content;
}

void send404(uWS::HttpResponse<false>* res) {
    res->writeStatus("404 Not Found");
    res->writeHeader("Content-Type", "text/plain");
    res->end("404 Not Found");
}

void send500(uWS::HttpResponse<false>* res, const std::string& message) {
    res->writeStatus("500 Internal Server Error");
    res->writeHeader("Content-Type", "text/plain");
    res->end("500 Internal Server Error: " + message);
}

int main(int argc, char* argv[]) {
    // Initialize runfiles
    std::string error;
    runfiles = rules_cc::cc::runfiles::Runfiles::Create(argv[0], &error);
    if (runfiles == nullptr) {
        std::cerr << "Error: Failed to initialize runfiles: " << error << std::endl;
        std::cerr << "Cannot serve files without runfiles access" << std::endl;
        return 1;
    }
    
    std::cout << "Starting static file server..." << std::endl;
    std::cout << "Workspace: wasm-bazel" << std::endl;
    
    uWS::App()
        .get("/*", [](auto *res, auto *req) {
            std::string urlPath = std::string(req->getUrl());
            std::cout << "Request for: " << urlPath << std::endl;
            
            // Default to index.html if root path
            if (urlPath == "/") {
                urlPath = "/index.html";
            }
            
            // Get the absolute file path
            std::string filePath = getRunfilesPath(urlPath);
            std::cout << "Resolved path: " << (filePath.empty() ? "EMPTY" : filePath) << std::endl;
            
            if (filePath.empty()) {
                // Path traversal or invalid path
                send404(res);
                return;
            }
            
            // Check if file exists and is readable
            if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) {
                send404(res);
                return;
            }
            
            // Read the file content
            std::string content = readFileContent(filePath);
            if (content.empty()) {
                send500(res, "Failed to read file");
                return;
            }
            
            // Determine MIME type
            std::string mimeType = getMimeType(filePath);
            
            // Send the response
            res->writeStatus("200 OK");
            res->writeHeader("Content-Type", mimeType);
            res->writeHeader("Content-Length", std::to_string(content.size()));
            res->end(content);
            
            std::cout << "Served: " << urlPath << " (" << mimeType << ", " 
                      << content.size() << " bytes)" << std::endl;
        })
        .listen("0.0.0.0", 8080, [](auto *token) {
            if (token) {
                std::cout << "Static file server listening on http://0.0.0.0:8080" << std::endl;
                std::cout << "Serving files from runfiles directory" << std::endl;
            } else {
                std::cerr << "Failed to start server" << std::endl;
            }
        })
        .run();
    
    return 0;
}