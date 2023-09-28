#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

class DB
{
public:
	DB(const std::string&);
	~DB();
	bool open();
	void close();
	bool query(const std::string&, std::vector<std::string>&);
	std::string escapeString(const std::string&);
	std::vector<std::string> retrieveUrlsToCrawl();
	sqlite3* getDbPtr() const;
private:
	sqlite3* db;
	std::string name;
};

DB::DB(const std::string& dbName)
	: name {dbName}, db{nullptr}
{
}

DB::~DB()
{
	if (db)
	{
		close();
	}
}

sqlite3* DB::getDbPtr() const {
    return db;
}

bool DB::open()
{
	int rc = sqlite3_open(name.c_str(), &db);
	if (rc != SQLITE_OK)
	{
		const char* errorMsg = sqlite3_errmsg(db);
        std::cerr << "Error opening database: " << errorMsg << std::endl;
		return false;
	}
	return true;
}

void DB::close()
{
	if (db)
	{
		sqlite3_close(db);
		db = nullptr;
	}
}

bool DB::query(const std::string& sql, std::vector<std::string>& results)
{
	if (!db)
		return false;
	char* errMsg = nullptr;
	int rc = sqlite3_exec(db, sql.c_str(), [](void* data, int argc, char** argv, char** azColName) -> int {
        std::vector<std::string>& results = *reinterpret_cast<std::vector<std::string>*>(data);
        for (int i = 0; i < argc; ++i) {
            results.push_back(argv[i] ? argv[i] : "NULL");
        }
        return 0;
    }, &results, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQLite error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

std::string DB::escapeString(const std::string& input) 
{   
	// For simplicity, you can start with basic escaping
    std::string escaped = input;
    size_t pos = 0;
    while ((pos = escaped.find('\'', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "''"); // Replace ' with ''
        pos += 2; // Move past the last '
    }
    return escaped;
}

std::vector<std::string> DB::retrieveUrlsToCrawl() {
    std::vector<std::string> urls;

    // SQL query to retrieve URLs from the web_pages table that need to be crawled
    const char* query = "SELECT url FROM web_pages WHERE crawled = 0"; // Assuming you have a 'crawled' column to track the status

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);

    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            // Retrieve the URL from the query result
            const char* url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            urls.push_back(url);
        }
        sqlite3_finalize(stmt);
    } else {
        throw std::runtime_error{"Error."};
    }

    return urls;
}

#endif // DB_H