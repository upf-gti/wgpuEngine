#pragma once

#include "includes.h"

#include <string>
#include <vector>

std::string remove_special_characters(const std::string& str);
std::vector<std::string> tokenize(const std::string& str, char token = ' ');

void to_camel_case(std::string& str);

bool read_file(const std::string& filename, std::string& content);

// https://stackoverflow.com/a/8518855
std::string dirname_of_file(const std::string& fname);

std::string generate_unique_id(uint32_t num_characters = 6u);
