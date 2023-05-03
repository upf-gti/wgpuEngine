#pragma once

#include <cassert>
#include <iostream>

#define assert_msg(condition, msg) if (!(condition)) {std::cout << msg << std::endl; assert(false);}