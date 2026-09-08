#include "error.h"

#include <array>

const char* error_names[] = {
    "OK", // OK
    "Failed", // FAILED
    "In Progress" // IN_PROGRESS
};

static_assert(std::size(error_names) == ERR_MAX);
