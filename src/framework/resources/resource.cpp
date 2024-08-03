#include "resource.h"

#include <cassert>

Resource::Resource()
{
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
