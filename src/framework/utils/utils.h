#pragma once

#include <vector>
#include <string>

enum Error {
    OK,
    FAILED,
    ERR_NOT_FOUND,
    ERR_CANT_CREATE,
    ERR_MAX,
};

#define assert_msg(condition, msg) if (!(condition)) { spdlog::error(msg); assert(false);}
#define _STR(m_x) #m_x

std::string remove_special_characters(const std::string& str);
std::vector<std::string> tokenize(const std::string & str, char token = ' ');

void print_line(const char* line);
void print_line(const std::string & line);
void print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message);

void to_camel_case(std::string& str);

bool read_file(const std::string & filename, std::string & content);

#define ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg)                                                                                     \
	if (m_cond) {																														 \
		print_error(__FUNCTION__, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true. Returning: " _STR(m_retval), m_msg);		 \
		return m_retval;                                                                                                                 \
	}


std::string delete_until_tags(std::istringstream& stream, std::string& text, std::streampos& current_pos, std::string& current_line, const std::vector<std::string>& tags);
std::string continue_until_tags(std::istringstream& stream, std::streampos& current_pos, std::string& current_line, const std::vector<std::string>& tags);

// https://stackoverflow.com/a/8518855
std::string dirname_of_file(const std::string& fname);
