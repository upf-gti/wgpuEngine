#include "utils.h"

#include <fstream>
#include <random>
#include <algorithm>

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
        const std::string new_token = std::string(start, next);
        if (!new_token.empty() && new_token != " " && new_token != "\t") {
            results.push_back(new_token);
        }
        start = next + 1;
        next = std::find(start, end, token);
    }

    results.push_back(remove_special_characters(std::string(start, next)));

    return results;
}

void print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message) {
    spdlog::error("{}(line {}) at {}\n{} - {}", p_function, p_line, p_file, p_error, p_message);
}

void to_camel_case(std::string& str)
{
    if (!str.size()) {
        return;
    }

    if (str[0] == '_') {
        str.erase(0, 1);
    }

    if (str.size() > 1) {
        str[0] = std::toupper(str[0]);
    }

    size_t num_chars = str.size();

    for (size_t i = 0; i < num_chars; ++i) {
        auto& c = str[i];
        if (c == '_') {
            if (i < num_chars - 1) {
                c = ' ';
                str[i + 1] = std::toupper(str[i + 1]);
            }
            else {
                str.erase(i);
            }
        }
    }
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

std::string dirname_of_file(const std::string& fname)
{
    size_t pos = fname.find_last_of("\\/");
    return (std::string::npos == pos)
        ? ""
        : fname.substr(0, pos);
}

std::string generate_unique_id(uint32_t num_characters)
{
    static std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static std::random_device random_device;
    static std::mt19937 generator(random_device());

    std::uniform_int_distribution<> distribution(0, characters.size() - 1);
    std::string unique_id = "";

    for (uint32_t i = 0; i < num_characters; i++) {
        unique_id += characters[distribution(generator)];
    }

    return unique_id;
}
