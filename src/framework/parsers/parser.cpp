#include "parser.h"

#include <chrono>
#include <functional>

void Parser::parse_async(const char* file_path, std::function<void()> callback, uint32_t flags)
{
    async_callback = callback;

    // Start the async operation
    async_future = std::async(std::launch::async, &Parser::parse, this, file_path, std::ref(async_entities), flags);
}

bool Parser::poll_async()
{
    // Poll to check if the task is done
    if (async_future.valid() && async_future.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
        async_callback();
        async_future.get();
        return true;
    }

    return false;
}
