#pragma once

#include "framework/math.h"
#include "framework/aabb.h"

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <functional>

using FuncEmpty = std::function<void()>;
using FuncVoid = std::function<void(const std::string&, void*)>;
using FuncFloat = std::function<void(const std::string&, float)>;
using FuncString = std::function<void(const std::string&, std::string)>;
using FuncVec2 = std::function<void(const std::string&, glm::vec2)>;
using FuncVec3 = std::function<void(const std::string&, glm::vec3)>;
using FuncVec4 = std::function<void(const std::string&, glm::vec4)>;

using SignalType = std::variant <FuncFloat, FuncString, FuncVec2, FuncVec3, FuncVec4, FuncVoid>;

enum NodeType {
    NODE_2D,
    NODE_3D
};

class Node {

    static std::map<std::string, std::vector<SignalType>> mapping_signals;
    static std::map<uint8_t, std::vector<FuncEmpty>> controller_signals;

protected:

	std::string name;

    NodeType type;

	std::vector<Node*> children;

    AABB aabb = {};

public:

    Node() {};
	virtual ~Node() {};

	virtual void render();
	virtual void update(float delta_time);

    std::string get_name() const { return name; }
    std::vector<Node*>& get_children() { return children; }
    AABB get_aabb() const;

    void set_name(std::string name) { this->name = name; }
    void set_aabb(const AABB& aabb) { this->aabb = aabb; }

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
