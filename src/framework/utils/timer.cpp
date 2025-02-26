#include "timer.h"

#include "spdlog/spdlog.h"

void Timer::start()
{
    begin = std::chrono::steady_clock::now();
}

void Timer::print_elapsed_time_s()
{
    if (begin == std::chrono::steady_clock::time_point()) {
        spdlog::error("Timer was not started!");
    }

    spdlog::info("Time elapsed: {} [s]", get_elapsed_time<std::ratio<1,1>>());
}

void Timer::print_elapsed_time_ms()
{
    if (begin == std::chrono::steady_clock::time_point()) {
        spdlog::error("Timer was not started!");
    }

    spdlog::info("Time elapsed: {} [ms]", get_elapsed_time<std::milli>());
}

void Timer::print_elapsed_time_ns()
{
    if (begin == std::chrono::steady_clock::time_point()) {
        spdlog::error("Timer was not started!");
    }

    spdlog::info("Time elapsed: {} [ns]", get_elapsed_time<std::nano>());
}
