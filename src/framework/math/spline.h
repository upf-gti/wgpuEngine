#pragma once

#include "math_utils.h"

#include <vector>
#include <functional>

struct Knot {
    glm::vec3 position;
    float size = 0.f;

    inline Knot operator+(const Knot& knot) const { return { position + knot.position }; }
    inline Knot operator-(const Knot& knot) const { return { position - knot.position }; }
    inline Knot operator*(const Knot& knot) const { return { position * knot.position }; }

};

class Spline {

protected:

    uint32_t density = 48;

    std::vector<glm::vec3> points;

    virtual glm::vec3 evaluate(float t) const { return {}; };

public:

    Spline() {};

    virtual void add_point(const glm::vec3& new_point);

    virtual void for_each(std::function<void(const glm::vec3&)> fn) {}

    uint32_t count() const { return points.size(); }

    void set_density(uint32_t new_density) { density = new_density; }
};

struct BezierSpline : public Spline {

    glm::vec3 evaluate_quadratic(float t, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) const;

    glm::vec3 evaluate_cubic(float t, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) const;

    std::vector<glm::vec3> compute_control_points(uint32_t segments);

    std::vector<glm::vec3> construct_target_vector(uint32_t n);
    std::vector<float> construct_lower_diagonal_vector(uint32_t length);
    std::vector<float> construct_main_diagonal_vector(uint32_t n);
    std::vector<float> construct_upper_diagonal_vector(uint32_t length);

public:

    BezierSpline() {

        density = 12;

        points = {
            glm::vec3(0.0f, 0.404f, 0.0f) * 0.5f,
            glm::vec3(0.0f, -0.068f, 0.0f) * 0.5f,
            glm::vec3(0.205f, 0.344f, 0.0f) * 0.5f,
            glm::vec3(0.324f, 0.074f, 0.0f) * 0.5f,
            glm::vec3(0.43f, 0.2f, 0.0f) * 0.5f,
            glm::vec3(0.58f, 0.467f, 0.0f) * 0.5f,
            glm::vec3(0.882f, 0.31f, 0.0f) * 0.5f
        };
    }

    void for_each(std::function<void(const glm::vec3&)> fn) override;
};

class BSpline : public Spline {

protected:

    uint32_t degree = 3;

    std::vector<float> knots;

    float get_basis(int i, int k, float t, const std::vector<float>& knots) const;

    glm::vec3 evaluate(float t) const override;

public:

    BSpline() {};

    void add_point(const glm::vec3& new_point) override;

    void for_each(std::function<void(const glm::vec3&)> fn) override;
};

class NURBS : public BSpline {

    std::vector<float> weights;

    glm::vec3 evaluate(float t) const override;

public:

    NURBS() {};

    void add_weighted_point(const glm::vec3& new_point, float weight);
};
