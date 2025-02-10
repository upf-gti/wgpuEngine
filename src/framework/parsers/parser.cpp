#include "parser.h"

#include <chrono>
#include <functional>

std::vector<Parser*> Parser::async_parsers;

void Parser::poll_async_parsers()
{
    std::vector<Parser*>::iterator it = async_parsers.begin();
    while (it != async_parsers.end())
    {
        Parser* parser = *it;
        if (parser->async_future.valid() && parser->async_future.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {

            parser->on_async_finished();

            parser->async_callback(parser->async_nodes, parser->async_future.get());

            delete* it;
            it = async_parsers.erase(it);
        }
        else {
            it++;
        }
    }
}
