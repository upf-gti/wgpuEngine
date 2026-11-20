#include "error.h"

#include <array>

#include "core/managers/debug/debug_manager.h"

const char* error_names[] = {
    "OK", // OK
    "Failed", // FAILED
    "In Progress" // IN_PROGRESS
};

static_assert(std::size(error_names) == ERR_MAX);

void _err_print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message)
{
    LOG_ERROR("{} - {} [{}]: {}", p_function, p_file, p_line, p_error);
}
