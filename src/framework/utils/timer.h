#pragma once

#include <chrono>

class Timer {
    std::chrono::high_resolution_clock::time_point begin;

    template<typename T>
    double get_elapsed_time()
    {
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, T>(end - begin).count();
    }

public:

    void start();

    void print_elapsed_time_s();
    void print_elapsed_time_ms();
    void print_elapsed_time_ns();
};
