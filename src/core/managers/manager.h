#pragma once

#include <stdint.h>

#include "core/error/error.h"

#define MANAGER_DECLARE(class_name)                         \
private:                                                    \
    static inline class_name* singleton_instance = nullptr; \
    friend int main(int argc, char** argv);                 \
                                                            \
public:                                                     \
    inline static class_name* get_singleton()               \
    {                                                       \
        return singleton_instance;                          \
    }                                                       \
                                                            \
private:

class Manager {
public:
    Manager() = default;
    ~Manager() = default;

    virtual Error initialize() = 0;
    virtual Error finalize() = 0;

private:
    static Manager* singleton_instance;
};
