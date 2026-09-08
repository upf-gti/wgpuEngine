#pragma once

#include "stdint.h"

// Heavily inspired by Godot Engine error handling, check: https://github.com/godotengine/godot/blob/master/core/error/error_macros.h

enum Error : uint8_t {
    OK,
    FAILED,
    IN_PROGRESS,
    ERR_MAX
};

void _err_print_error(const char* p_function, const char* p_file, int p_line, const char* p_error, const char* p_message = nullptr);

#define ERROR_CHECK_INDEX(index, size)                                                                              \
    if (unlikely((index) < 0 || (index) >= (size))) {                                                               \
        _err_print_error(__FUNCTION__, __FILE__, __LINE__, "Index \"" #index "\" is out of bounds [0, " #size "]"); \
        return;                                                                                                     \
    } else                                                                                                          \
        ((void)0)

#define ERR_FAIL_NULL(param)                                                                     \
    if (unlikely(param == nullptr)) {                                                            \
        _err_print_error(__FUNCTION__, __FILE__, __LINE__, "Parameter \"" #param "\" is null."); \
        return;                                                                                  \
    } else                                                                                       \
        ((void)0)

#define ERR_FAIL_NULL_MSG(param, msg)                                                                 \
    if (unlikely(param == nullptr)) {                                                                 \
        _err_print_error(__FUNCTION__, __FILE__, __LINE__, "Parameter \"" #param "\" is null.", msg); \
        return;                                                                                       \
    } else                                                                                            \
        ((void)0)
