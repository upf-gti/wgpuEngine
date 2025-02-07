#pragma once

#include "framework/math/aabb.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

using FuncEmpty = std::function<void()>;
using FuncVoid = std::function<void(const std::string&, void*)>;
using FuncFloat = std::function<void(const std::string&, float)>;
using FuncInt = std::function<void(const std::string&, int)>;
using FuncString = std::function<void(const std::string&, std::string)>;
using FuncVec2 = std::function<void(const std::string&, glm::vec2)>;
using FuncUVec2 = std::function<void(const std::string&, glm::u32vec2)>;
using FuncVec3 = std::function<void(const std::string&, glm::vec3)>;
using FuncVec4 = std::function<void(const std::string&, glm::vec4)>;

using SignalType = std::variant <FuncInt, FuncFloat, FuncString, FuncVec2, FuncUVec2, FuncVec3, FuncVec4, FuncVoid>;

class Node {

    static std::unordered_map<std::string, std::vector<SignalType>> mapping_signals;
    static std::unordered_map<uint8_t, std::vector<FuncEmpty>> controller_signals;

public:

    enum class AnimatablePropertyType {
        UNDEFINED,
        INT8,
        INT16,
        INT32,
        INT64,
        UINT8,
        UINT16,
        UINT32,
        UINT64,
        FLOAT32,
        FLOAT64,
        IVEC2,
        UVEC2,
        FVEC2,
        IVEC3,
        UVEC3,
        FVEC3,
        IVEC4,
        UVEC4,
        FVEC4,
        QUAT
    };

    struct AnimatableProperty {
        AnimatablePropertyType property_type = AnimatablePropertyType::UNDEFINED;
        void* property = nullptr;
        std::function<void()> fn = nullptr;
    };

protected:

    static uint32_t last_node_id;

    std::string name;
    std::string node_type;

    std::string scene_unique_id;

    Node* parent = nullptr;
    std::vector<Node*> children;

    AABB aabb = {};
    std::unordered_map<std::string, AnimatableProperty> animatable_properties;

public:

    Node();
    virtual ~Node() {};

    virtual void initialize() {};

    virtual void add_child(Node* child);
    virtual void remove_child(Node* child);

    virtual void render();
    virtual void update(float delta_time);
    virtual void render_gui() {};

    virtual void serialize(std::ofstream& binary_scene_file);
    virtual void parse(std::ifstream& binary_scene_file);

    void set_node_type(const std::string& new_type) { node_type = new_type; }
    void set_name(const std::string& new_name) { name = new_name; }
    virtual void set_aabb(const AABB& new_aabb) { aabb = new_aabb; }

    std::string get_name() const { return name; }
    std::string get_node_type() const { return node_type; }
    std::string get_scene_unique_id() const { return scene_unique_id; }
    virtual std::vector<Node*>& get_children() { return children; }
    virtual Node* get_node(std::vector<std::string>& path_tokens);
    AABB get_aabb() const;
    Node* get_node(const std::string& path);
    Node::AnimatableProperty& get_animatable_property(const std::string& name);
    const std::unordered_map<std::string, AnimatableProperty>& get_animatable_properties() const;

    template <typename T = Node*>
    T get_parent() const { if (!parent) return nullptr;  return dynamic_cast<T>(parent); };

    std::string find_path(const std::string& node_name, const std::string& current_path = "");

    virtual void disable_2d();

    virtual void clone(Node* new_node, bool copy = true);
    virtual void release();

    /*
    *	Callbacks
    */

    static void bind(const std::string& name, SignalType callback);
    static void bind_button(uint8_t button, FuncEmpty callback);

    static void unbind(const std::string& name);

    static void check_controller_signals();

    template<typename T>
    static bool emit_signal(const std::string& name, T value) {

        if (!mapping_signals.contains(name)) {
            return false;
        }

        using FuncT = std::function<void(const std::string&, T)>;

        for (auto& f : mapping_signals[name])
        {
            if (std::holds_alternative<FuncT>(f))
                std::get<FuncT>(f)(name, value);
        }

        return true;
    }
};

class NodeRegistry {

    std::unordered_map<std::string, std::function<Node* ()>> registry;

public:

    static NodeRegistry* instance;

    NodeRegistry();

    void register_class(const std::string& name, std::function<Node* ()> constructor);

    Node* create_node(const std::string& name);

    static NodeRegistry* get_instance() {
        if (instance == nullptr) {
            instance = new NodeRegistry();
        }
        return instance;
    }
};
