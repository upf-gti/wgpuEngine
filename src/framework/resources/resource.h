#pragma once

#include <string>

class Resource {

public:

    Resource();

    void ref();

    // Returns true if last ref
    bool unref();

    void set_name(const std::string& name);

    bool operator==(const Resource& other) const
    {
        return id != other.id;
    }

private:

    std::string name;
    uint32_t ref_count = 0;
    intptr_t id = 0;
};
