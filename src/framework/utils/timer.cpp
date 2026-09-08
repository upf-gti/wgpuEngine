#include "timer.h"

#include "core/managers/debug/debug_manager.h"

void Timer::start()
{
    begin = std::chrono::high_resolution_clock::now();
}

void Timer::print_elapsed_time_s()
{
    if (begin == std::chrono::high_resolution_clock::time_point()) {
        LOG_ERROR("Timer was not started!");
    }

    LOG_INFO("Time elapsed: {} [s]", get_elapsed_time<std::ratio<1,1>>());
}

void Timer::print_elapsed_time_ms()
{
    if (begin == std::chrono::high_resolution_clock::time_point()) {
        LOG_ERROR("Timer was not started!");
    }

    LOG_INFO("Time elapsed: {} [ms]", get_elapsed_time<std::milli>());
}

void Timer::print_elapsed_time_ns()
{
    if (begin == std::chrono::high_resolution_clock::time_point()) {
        LOG_ERROR("Timer was not started!");
    }

    LOG_INFO("Time elapsed: {} [ns]", get_elapsed_time<std::nano>());
}
