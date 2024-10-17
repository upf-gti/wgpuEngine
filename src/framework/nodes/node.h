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
using FuncVec3 = std::function<void(const std::string&, glm::vec3)>;
using FuncVec4 = std::function<void(const std::string&, glm::vec4)>;

using SignalType = std::variant <FuncInt, FuncFloat, FuncString, FuncVec2, FuncVec3, FuncVec4, FuncVoid>;

enum NodeType {
    NODE_2D,
    NODE_3D,
    JOINT_3D
};

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
    };

protected:

    static uint32_t last_node_id;

    std::string name;
    std::string node_type;

    NodeType type;

    std::vector<Node*> children;

    AABB aabb = {};
    std::unordered_map<std::string, AnimatableProperty> animatable_properties;

public:

    Node();
    virtual ~Node() {};

    virtual void render();
    virtual void update(float delta_time);
    virtual void render_gui() {};

    virtual void serialize(std::ofstream& binary_scene_file);
    virtual void parse(std::ifstream& binary_scene_file);

    std::string get_name() const { return name; }
    NodeType get_type() const { return type; }
    std::string get_node_type() const { return node_type; }
    virtual std::vector<Node*>& get_children() { return children; }
    AABB get_aabb() const;

    virtual Node* get_node(std::vector<std::string>& path_tokens);
    Node* get_node(const std::string& path);
    std::string find_path(const std::string& node_name, const std::string& current_path = "");

    Node::AnimatableProperty get_animatable_property(const std::string& name);
    const std::unordered_map<std::string, AnimatableProperty>& get_animatable_properties() const;

    void set_type(NodeType new_type) { type = new_type; }
    void set_name(std::string new_name) { name = new_name; }
    virtual void set_aabb(const AABB& new_aabb) { aabb = new_aabb; }

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

        auto it = mapping_signals.find(name);
        if (it == mapping_signals.end())
            return false;

        using FuncT = std::function<void(const std::string&, T)>;

        for (auto& f : mapping_signals[name])
        {
            if (std::holds_alternative<FuncT>(f))
                std::get<FuncT>(f)(name, value);
        }

        return true;
    }
};

class NodeFactory {
public:

    static NodeFactory* instance;

    std::unordered_map<std::string, std::function<Node* ()>> registry;

    NodeFactory();

    void register_class(const std::string& name, std::function<Node* ()> constructor) {
        registry[name] = constructor;
    }

    Node* create_node(const std::string& name) {
        if (registry.find(name) != registry.end()) {
            return registry[name]();
        }
        return nullptr;
    }

    static NodeFactory* get_instance() {
        if (instance == nullptr) {
            instance = new NodeFactory();
        }
        return instance;
    }
};

#define REGISTER_NODE_TYPE(node_type) \
    NodeFactory::get_instance()->register_class(#node_type, []() -> Node* { return new node_type(); });
