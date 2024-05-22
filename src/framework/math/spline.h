#pragma once

#include "math_utils.h"

#include <vector>
#include <functional>

struct Knot {
    glm::vec3 position;
    float size = 1.f;
};

inline Knot operator+(const Knot& a, const Knot& b) { return { a.position + b.position, a.size + b.size }; }
inline Knot operator-(const Knot& a, const Knot& b) { return { a.position - b.position, a.size - b.size }; }
inline Knot operator*(const Knot& a, const Knot& b) { return { a.position * b.position, a.size * b.size }; }
inline Knot operator/(const Knot& a, const Knot& b) { return { a.position / b.position, a.size / b.size }; }

inline Knot operator*(float scalar, const Knot& b) { return { scalar * b.position, scalar * b.size }; }
inline Knot operator*(const Knot& a, float scalar) { return { a.position * scalar, a.size * scalar }; }

class Spline {

protected:

    uint32_t density = 32;

    std::vector<Knot> knots;

    virtual Knot evaluate(float t) const { return {}; };

public:

    Spline() {};

    virtual void add_knot(const Knot& new_knot);

    virtual void for_each(std::function<void(const Knot&)> fn) {}

    uint32_t count() const { return knots.size(); }

    void set_density(uint32_t new_density) { density = new_density; }
};

struct BezierSpline : public Spline {

    float knot_distance = 0.01f;

    std::vector<std::vector<float>> luts;

    std::vector<Knot> control_points;

    Knot evaluate_quadratic(float t, const Knot& p0, const Knot& p1, const Knot& p2) const;
    Knot evaluate_cubic(float t, const Knot& p0, const Knot& p1, const Knot& p2, const Knot& p3) const;

    void compute_luts(uint32_t segments);
    void compute_control_points(uint32_t segments);

    std::vector<Knot> construct_target_vector(uint32_t n);
    std::vector<float> construct_lower_diagonal_vector(uint32_t length);
    std::vector<float> construct_main_diagonal_vector(uint32_t n);
    std::vector<float> construct_upper_diagonal_vector(uint32_t length);

    void fill_lut(uint32_t idx, uint32_t segments, Knot& start_point);

    float sample_lut(uint32_t idx, float f);

public:

    BezierSpline() {

        knots = {
            Knot({ 0.0f, 0.7f, 0.0f }, 2.0f) * 0.5f,
            Knot({ 0.0f, -0.068f, 0.0f }) * 0.5f,
            Knot({ 0.205f, 0.344f, 0.0f }) * 0.5f,
            Knot({ 0.324f, 0.074f, 0.0f }, 2.0f) * 0.5f,
            Knot({ 0.43f, 0.2f, 0.0f }) * 0.5f,
            Knot({ 0.58f, 0.467f, 0.0f }) * 0.5f,
            Knot({ 0.882f, 0.31f, 0.0f }, 3.0f) * 0.5f,
            Knot({ 0.682f, 0.121f, 0.0f }, 3.0f) * 0.5f
        };
    }

    void add_knot(const Knot& new_knot) override;

    void for_each(std::function<void(const Knot&)> fn) override;
};

//class BSpline : public Spline {
//
//protected:
//
//    uint32_t degree = 3;
//
//    std::vector<float> knots;
//
//    float get_basis(int i, int k, float t, const std::vector<float>& knots) const;
//
//    glm::vec3 evaluate(float t) const override;
//
//public:
//
//    BSpline() {};
//
//    void add_point(const glm::vec3& new_point) override;
//
//    void for_each(std::function<void(const glm::vec3&)> fn) override;
//};
//
//class NURBS : public BSpline {
//
//    std::vector<float> weights;
//
//    glm::vec3 evaluate(float t) const override;
//
//public:
//
//    NURBS() {};
//
//    void add_weighted_point(const glm::vec3& new_point, float weight);
//};
