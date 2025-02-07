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

    virtual void on_delete();

    void set_name(const std::string& name);
    const std::string& get_name() const;

    bool operator==(const Resource& other) const
    {
        return scene_unique_id == other.scene_unique_id;
    }

    void* get_property(const std::string& name);

    std::string get_scene_unique_id() const { return scene_unique_id; }

private:

    uint32_t ref_count = 0;
    std::string scene_unique_id = "";

protected:

    std::string name = "";

    // animatable properties
    std::unordered_map<std::string, void*> properties;
};
