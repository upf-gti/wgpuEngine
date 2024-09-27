#pragma once

#include <string>
#include <unordered_map>

class Resource {

public:

    Resource();
    virtual ~Resource() {}

    void ref();

    // Returns true if last ref
    bool unref();

    void set_name(const std::string& name);
    const std::string& get_name() const;

    bool operator==(const Resource& other) const
    {
        return id == other.id;
    }

    void* get_property(const std::string& name);

private:

    uint32_t ref_count = 0;
    intptr_t id = 0;

protected:

    std::string name = "";

    // animatable properties
    std::unordered_map<std::string, void*> properties;
};
