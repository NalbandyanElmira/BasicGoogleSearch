#ifndef INDEXER_H
#define INDEXER_H

#include "db.h"
#include "validWords.h"

#include <fstream>
#include <stdexcept>
#include <regex>
#include <algorithm>
#include <unordered_map>
#include <iostream>

class Indexer
{
public:
	Indexer(DB&);
	void indexWordsFromHTMLFile(const std::string&);
	void printMap();
	void saveDataToDatabase();
	void generateSearchResultFiles(const std::string&);
private:
	DB db;
	ValidWords vld;
	std::vector<std::string> validWords;
	std::unordered_map<std::string, std::vector<std::string>> wordToFile;

	std::string removeHtmlTags(const std::string&);
	std::string normalizedWord(const std::string&);
	std::string generateSearchResultHTML(const std::string&);
};

Indexer::Indexer(DB& d)
	: db{d}
	, vld{"include/words_alpha.txt"}
{
	validWords = vld.getValidWords();
}

void Indexer::printMap()
{
	for (const auto& entry : wordToFile) {
        const std::string& word = entry.first;
        const std::vector<std::string>& files = entry.second;

        std::cout << "Word: " << word << std::endl;
        std::cout << "Files where it appears: ";
        for (const std::string& file : files) {
            std::cout << file << " ";
        }
        std::cout << std::endl;
    }
}

int binarySearch(const std::vector<std::string>& wordsAlpha, const std::string& target) {
    int start = 0;
    int end = wordsAlpha.size() - 1;

    while (start <= end) {
        int mid = start + (end - start) / 2;
        if (wordsAlpha[mid] == target) {
            return mid;
        } 
        else if (target > wordsAlpha[mid]) {
            start = mid + 1;
        } 
        else {
            end = mid - 1;
        }
    }
    return -1;
}


void Indexer::indexWordsFromHTMLFile(const std::string& fileName)
{
	std::ifstream file(fileName);
	if (!file.is_open())
	{
		throw std::runtime_error{"Can't open the file " + fileName + "."};
	}
	std::string pageContent;
    std::string line;
    while (std::getline(file, line))
    {
        pageContent += line;
    }
    std::string textContent = removeHtmlTags(pageContent);
    // Tokenize text into words
    std::istringstream iss(textContent);
    std::string word;
    while (iss >> word)
    {
        // Normalize the word
        std::string normalized = normalizedWord(word);
        // Check if the normalized word is a valid English word (you'll have your list)
        if (binarySearch(validWords, normalized) != -1)
        {
            // Map the word to the current file name
            auto& files = wordToFile[normalized];
            if (std::find(files.begin(), files.end(), fileName) == files.end())
            {
                files.push_back(fileName);
            }
        }
    }

    file.close();
}

std::string Indexer::normalizedWord(const std::string& word)
{
	std::string w;
	for (int i = 0; i < word.size(); ++i)
	{
		if (isalpha(word[i]))
			w += std::tolower(word[i]);
	}
	return w;
}

std::string Indexer::removeHtmlTags(const std::string& input)
{
	// Use regular expressions to remove HTML tags
    std::regex pattern("<.*?>");
    return std::regex_replace(input, pattern, "");
}

void Indexer::saveDataToDatabase()
{
    if (db.open())
    {
        // Create tables if they don't exist
        std::string createWordsTableSQL = "CREATE TABLE IF NOT EXISTS words (id INTEGER PRIMARY KEY, word TEXT UNIQUE);";
        std::string createFilesTableSQL = "CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY, name TEXT UNIQUE);";
        std::string createFileWordRelationshipsTableSQL = "CREATE TABLE IF NOT EXISTS file_word_relationships (id INTEGER PRIMARY KEY, file_id INTEGER, word_id INTEGER, FOREIGN KEY (word_id) REFERENCES words (id), FOREIGN KEY (file_id) REFERENCES files (id));";
        std::vector<std::string> res;
        db.query(createWordsTableSQL, res);
        db.query(createFilesTableSQL, res);
        db.query(createFileWordRelationshipsTableSQL, res);

        // Iterate through wordToFile and insert data into the database
        for (const auto& entry : wordToFile)
        {
            const std::string& word = entry.first;
            const std::vector<std::string>& files = entry.second;

            // Insert the word into the 'words' table
            std::string insertWordSQL = "INSERT INTO words (word) VALUES ('" + word + "');";
            db.query(insertWordSQL, res);

            // Insert the file into the 'files' table
            for (const std::string& file : files)
            {
            	std::string insertFileSQL = "INSERT INTO files (name) VALUES ('" + file + "');";
            	db.query(insertFileSQL, res);
            }

            // Insert file-word relationships into the 'file_word_relationships' table
            for (const std::string& file : files)
            {
                std::string insertRelationshipSQL = "INSERT INTO file_word_relationships (file_id, word_id) VALUES ((SELECT id FROM files WHERE name = '" + file + "'), (SELECT id FROM words WHERE word = '" + word + "'));";
                db.query(insertRelationshipSQL, res);
            }
        }

        db.close();
    }
    else
    {
        std::cerr << "Failed to open the database." << std::endl;
    }
}

void Indexer::generateSearchResultFiles(const std::string& outputDirectory)
{
    for (const auto& entry : wordToFile)
    {
        const std::string& word = entry.first;
        const std::string htmlContent = generateSearchResultHTML(word);
        const std::string filename = outputDirectory + "/search_result_" + word + ".html";

        std::ofstream outputFile(filename);
        if (outputFile.is_open())
        {
            outputFile << htmlContent;
            outputFile.close();
        }
        else
        {
            std::cerr << "Error creating " << filename << std::endl;
        }
    }
}

std::string Indexer::generateSearchResultHTML(const std::string& word)
{
    std::stringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "<title>Search Result for " << word << "</title>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "<h1>Search Result for " << word << "</h1>\n";
    
    html << "<p>This word appears in the following files:</p>\n";
    html << "<ul>\n";
    
    const std::vector<std::string>& files = wordToFile[word];
    for (const std::string& file : files)
    {
        // Create a link to each file
        html << "<li><a href='" << "../" << file << "'>" << word << "</a></li>\n";
    }
    
    html << "</ul>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}

#endif // INDEXER_H