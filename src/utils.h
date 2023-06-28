#pragma once

#include <regex>
#include <cassert>
#include <iostream>
#include <fstream>

#define assert_msg(condition, msg) if (!(condition)) {std::cout << msg << std::endl; assert(false);}
#define _STR(m_x) #m_x

/*
*	Math
*/

float clampf(float num, float left, float right) {
	if (num < left)
		num = left;
	else if (num > right)
		num = right;
	return num;
}

/*
*	Other
*/

inline std::vector<std::string> tokenize(const std::string& str) {
	
	std::regex reg("\\s+");

	std::sregex_token_iterator iter(str.begin(), str.end(), reg, -1);
	std::sregex_token_iterator end;

	std::vector<std::string> vec(iter, end);
	return vec;
}

inline void printLine(const char* line) {
	std::cout << line << std::endl;
}

inline void printLine(const std::string & line) {
	std::cout << line << std::endl;
}

inline void printError(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message) {
	std::cout << p_function << "(line " << p_line << ") at " << p_file << std::endl << p_error << " - " << p_message << std::endl;
}

inline bool readFile(const std::string& filename, std::string& content)
{
	content.clear();

	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "File not found " << filename << std::endl;
		return false;
	}

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	content.resize(size, ' ');

	file.seekg(0);
	file.read(content.data(), size);

	return true;
}

enum Error {
	OK,
	FAILED,
	ERR_NOT_FOUND,
	ERR_CANT_CREATE,
	ERR_MAX,
};

#define ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg)                                                                                     \
	if (m_cond) {																														 \
		printError(__FUNCTION__, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Returning: " _STR(m_retval), m_msg);		 \
		return m_retval;                                                                                                                 \
	}