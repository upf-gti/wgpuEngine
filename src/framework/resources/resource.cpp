#include "resource.h"

#include "framework/utils/utils.h"

#include "spdlog/spdlog.h"

#include <cassert>

Resource::Resource()
{
    // TODO: set proper unique id for project save/load
    // id = reinterpret_cast<intptr_t>(this);

    scene_unique_id = generate_unique_id();
}

void Resource::ref()
{
    ref_count++;
}

bool Resource::unref()
{
    assert(ref_count > 0u);

    ref_count--;

    if (ref_count == 0u) {
        on_delete();
        return true;
    }

    return false;
}

void Resource::on_delete()
{
    delete this;
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
