#ifndef QUERYPROCESSOR_H
#define QUERYPROCESSOR_H

#include "db.h"

const int MAX_INPUT_LENGTH = 20; 

class QueryProcessor {
public:
    QueryProcessor(DB& db) : db(db) {}

    std::string processQuery(const std::string& query) {
        // Sanitize user input
        std::string sanitizedQuery = sanitizeInput(query);
        // Formulate an SQL query using placeholders
        std::string sqlQuery = formulateSQLQuery(sanitizedQuery);
        std::vector<std::string> res;
        if (db.open()) {
            std::vector<std::string> res;
            if (db.query(sqlQuery, res)) {
                // Query executed successfully
                return generateHTMLResponse(res);
            } else {
                // Error executing the query
                std::cerr << "Error executing the query.\n";
                return "Error executing the query.";
            }
        } else {
            // Error opening the database
            std::cerr << "Error opening the database.\n";
            return "Error opening the database.";
        }
    }

private:
    DB db;

    std::string sanitizeInput(const std::string& input) {
        std::string sanitized = input.substr(0, MAX_INPUT_LENGTH);

        // Escape special characters to prevent SQL injection
        sanitized = db.escapeString(sanitized);

        return sanitized;
    }

    std::string formulateSQLQuery(const std::string& sanitizedQuery) {
        // Formulate an SQL query to retrieve file_ids associated with the given word
        return "SELECT f.name FROM files f "
           "JOIN file_word_relationships fwr ON f.id = fwr.file_id "
           "JOIN words w ON w.id = fwr.word_id "
           "WHERE w.word = '" + sanitizedQuery + "'";
    }

    std::string generateHTMLResponse(const std::vector<std::string>& results) {
        // Generate an HTML response to display search results
        std::string response = "<html><head><title>Search Results</title></head><body>";
        response += "<h1>Search Results</h1>";

        if (!results.empty()) {
            for (const std::string& link : results) {
                response += "<a href='" + link + "'>" + link + "</a><br>";
            }
        } else {
            response += "No results found.";
        }

        response += "</body></html>";
        return response;
    }
};

#endif // QUERYPROCESSOR_H