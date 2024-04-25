#include "utils.h"

#include <fstream>

#include "spdlog/spdlog.h"

std::string remove_special_characters(const std::string& str)
{
    std::string final_str = str;
    final_str.erase(std::remove_if(final_str.begin(), final_str.end(),
        [](char c) { return c == '\r' || c == '\n'; }),
        final_str.end());

    return final_str;
}

std::vector<std::string> tokenize(const std::string& str, char token)
{
	std::vector<std::string> results;
    std::string::const_iterator start = str.begin();
    std::string::const_iterator end = str.end();
    std::string::const_iterator next = std::find(start, end, token);
    while (next != end) {
        results.push_back(std::string(start, next));
        start = next + 1;
        next = std::find(start, end, token);
    }

    results.push_back(remove_special_characters(std::string(start, next)));

    return results;
}

void print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message) {
    spdlog::error("{}(line {}) at {}\n{} - {}", p_function, p_line, p_file, p_error, p_message);
}

bool read_file(const std::string& filename, std::string& content)
{
	content.clear();

	std::ifstream file(filename);
	if (!file.is_open()) {
        spdlog::error("Error reading file ({}): {}", filename, strerror(errno));
		return false;
	}

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	content.resize(size, ' ');

	file.seekg(0);
	file.read(content.data(), size);

    file.close();

	return true;
}


std::string delete_until_tags(std::istringstream& stream, std::string& text, std::streampos& current_pos, std::string& current_line, const std::vector<std::string>& tags)
{
    std::string current_tag;

    while (true) {
        std::getline(stream, current_line);
        auto tokens = tokenize(current_line);
        current_tag = tokens[0];

        auto it = std::find(tags.begin(), tags.end(), current_tag);

        if (it != tags.end()) {
            return *it;
        }
        else {
            text.replace(current_pos, current_line.length() + 1, "");
        }
    }
}

std::string continue_until_tags(std::istringstream& stream, std::streampos& current_pos, std::string& current_line, const std::vector<std::string>& tags)
{
    std::string current_tag;

    while (true) {
        std::getline(stream, current_line);
        auto tokens = tokenize(current_line);
        current_tag = tokens[0];

        auto it = std::find(tags.begin(), tags.end(), current_tag);

        if (it != tags.end()) {
            return *it;
        }
        else {
            current_pos += current_line.length() + 1;
        }
    }

    return "";
}

std::string dirname_of_file(const std::string& fname)
{
    size_t pos = fname.find_last_of("\\/");
    return (std::string::npos == pos)
        ? ""
        : fname.substr(0, pos);
}
