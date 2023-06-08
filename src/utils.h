#pragma once

#include <cassert>
#include <iostream>

#define assert_msg(condition, msg) if (!(condition)) {std::cout << msg << std::endl; assert(false);}
#define _STR(m_x) #m_x

inline void printLine(const char* line) {
	std::cout << line << std::endl;
}

inline void printLine(const std::string & line) {
	std::cout << line << std::endl;
}

inline void printError(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message) {
	std::cout << p_function << "(line " << p_line << ") at " << p_file << std::endl << p_error << " - " << p_message << std::endl;
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