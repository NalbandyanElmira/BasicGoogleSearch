#include "include/db.h"
#include "include/indexer.h"
#include "include/server.h"

#include <iostream>
#include <sqlite3.h>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <string>

std::string readHTMLFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string extractQueryFromRequest(const std::string& request) {
    size_t queryStart = request.find("?q=");
    if (queryStart != std::string::npos) {
        queryStart += 3; // Move past "GET /search?q="
        return request.substr(queryStart, -1);
    }
    return "";
}

std::string serveIndexPage(const std::string& request) {
    // Read the content of index.html from your file system
    std::string htmlContent = readHTMLFile("index.html");

    // Construct an HTTP response with the HTML content
    std::string httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(htmlContent.length()) + "\r\n\r\n" + htmlContent;

    return httpResponse;
}

std::string serveSearchPage(const std::string& request)
{
    // Extract the search query from the request
    std::string searchQuery = extractQueryFromRequest(request);

    // Check if a search query is provided
    if (!searchQuery.empty()) {
        // Assuming you have the corresponding HTML file for the search query
        std::string htmlFileName = "/search_results/search_result_" + searchQuery + ".html";

        // Generate an HTML response with redirection
        std::string httpResponse = "HTTP/1.1 302 Found\r\nLocation: /" + htmlFileName + "\r\n\r\n";
        return httpResponse;
    }

    std::string errorMessage = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n";
    errorMessage += "<html><body><h1>Bad Request: Invalid search query</h1></body></html>\r\n";
    
    return errorMessage;
}

int main() {
    DB myDB("Google.db");
    Indexer indexer(myDB);
    indexer.indexWordsFromHTMLFile("files/1.html");
    indexer.indexWordsFromHTMLFile("files/2.html");
    indexer.indexWordsFromHTMLFile("files/3.html");
    indexer.indexWordsFromHTMLFile("files/4.html");
    indexer.generateSearchResultFiles("search_results/");
    QueryProcessor qp(myDB);

    // Create a server with a specific port (e.g., 8080)
    Server server(8080, qp);

    server.addHandler("/", [&](const std::string& request) {
        return serveIndexPage(request);
    });

    server.addHandler("/search_results/search_result_algorithm.html", [](const std::string& request) {
        // Read the content of the HTML file and return it as the HTTP response
        std::string htmlContent = readHTMLFile("search_results/search_result_algorithm.html");
        
        if (!htmlContent.empty()) {
            // Construct an HTTP response with the HTML content
            std::string httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(htmlContent.length()) + "\r\n\r\n" + htmlContent;
            return httpResponse;
        } else {
            // Handle the case where the file couldn't be read or doesn't exist
            std::string errorMessage = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";
            errorMessage += "<html><body><h1>404 Not Found</h1></body></html>\r\n";
            return errorMessage;
        }
    });

    server.addHandler("/files/4.html", [&](const std::string& request) {
        // Extract the requested file path from the request
        std::string htmlContent = readHTMLFile("files/4.html");
        
        if (!htmlContent.empty()) {
            // Construct an HTTP response with the HTML content
            std::string httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(htmlContent.length()) + "\r\n\r\n" + htmlContent;
            return httpResponse;
        } else {
            // Handle the case where the file couldn't be read or doesn't exist
            std::string errorMessage = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";
            errorMessage += "<html><body><h1>404 Not Found</h1></body></html>\r\n";
            return errorMessage;
        }
    });


    // Start the server
    server.start();

    return 0;
}