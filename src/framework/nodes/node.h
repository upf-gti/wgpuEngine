#pragma once

#include "framework/aabb.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <functional>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/gtc/quaternion.hpp"

#include "framework/animation/bone_transform.h"

using FuncEmpty = std::function<void()>;
using FuncFloat = std::function<void(const std::string&, float)>;
using FuncString = std::function<void(const std::string&, std::string)>;
using FuncVec2 = std::function<void(const std::string&, glm::vec2)>;
using FuncVec3 = std::function<void(const std::string&, glm::vec3)>;
using FuncVec4 = std::function<void(const std::string&, glm::vec4)>;
using FuncQuat = std::function<void(const std::string&, glm::quat)>;
using FuncVoid = std::function<void(const std::string&, void*)>;
using FuncTransform = std::function<void(const std::string&, Transform)>;

using SignalType = std::variant <FuncFloat, FuncString, FuncVec2, FuncVec3, FuncVec4, FuncQuat, FuncVoid, FuncTransform>;

enum NodeType {
    NODE_2D,
    NODE_3D,
    JOINT_3D
};

class Node; 
class Node {

    static std::unordered_map<std::string, std::vector<SignalType>> mapping_signals;
    static std::unordered_map<uint8_t, std::vector<FuncEmpty>> controller_signals;
    static uint32_t last_node_id;

protected:

    std::string name;

    NodeType type;

    std::vector<Node*> children;

    AABB aabb = {};

    std::unordered_map<std::string, void*> properties;

public:

    Node();
    virtual ~Node() {};

    virtual void render();
    virtual void update(float delta_time);
    virtual void render_gui() {};

    NodeType get_type() const { return type; }
    std::string get_name() const { return name; }
    virtual std::vector<Node*>& get_children() { return children; }
    AABB get_aabb() const;

    virtual Node* get_node(std::vector<std::string>& path_tokens);
    Node* get_node(const std::string& path);
    std::string find_path(const std::string& node_name, const std::string& current_path = "");

    void* get_property(const std::string& name);

    void set_type(NodeType new_type) { type = new_type; }
    void set_name(std::string new_name) { name = new_name; }
    void set_aabb(const AABB& new_aabb) { aabb = new_aabb; }

    virtual void remove_flag(uint8_t flag);

    /*
    *	Callbacks
    */

    static void bind(const std::string& name, SignalType callback);
    static void bind(uint8_t button, FuncEmpty callback);

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
