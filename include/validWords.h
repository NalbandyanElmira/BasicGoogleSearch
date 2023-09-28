#ifndef VALIDWORDS_H
#define VALIDWORDS_H

#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

class ValidWords
{
public:
	ValidWords(const std::string&);
	std::vector<std::string> getValidWords() const;
private:
	std::string fileName;
	std::vector<std::string> validWords;

	void loadWords();
};

ValidWords::ValidWords(const std::string& f)
	: fileName{f}
{
	loadWords();
}

std::vector<std::string> ValidWords::getValidWords() const
{
	return validWords;
}

void ValidWords::loadWords()
{
	std::ifstream file(fileName);
	if (!file.is_open())
	{
		throw std::runtime_error{"Can't open the file."};
	}
	else
	{
		std::string word;
		while (std::getline(file, word))
		{
			size_t firstNonSpace = word.find_first_not_of(" \t\r\n");
            size_t lastNonSpace = word.find_last_not_of(" \t\r\n");

            if (firstNonSpace != std::string::npos && lastNonSpace != std::string::npos)
            {
                validWords.push_back(word.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1));
            }
		}
		file.close();
	}
}

#endif // VALIDWORDS_H