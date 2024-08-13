#include "resource.h"

#include <cassert>

Resource::Resource()
{
    // TODO: set proper unique id for project save/load
    id = reinterpret_cast<intptr_t>(this);
}

void Resource::ref()
{
    ref_count++;
}

bool Resource::unref()
{
    assert(ref_count > 0);

    ref_count--;

    if (ref_count == 0) {
        delete this;
        return true;
    }

    return false;
}

void Resource::set_name(const std::string& name)
{
    this->name = name;
}

const std::string& Resource::get_name() const
{
    return name;
}

void* Resource::get_property(const std::string& name)
{
    if (properties.contains(name)) {
        return properties[name];
    }

    return nullptr;
}
