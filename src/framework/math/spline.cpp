#include "spline.h"

#include <glm/gtx/quaternion.hpp>

// Splines

void Spline::add_knot(const Knot& new_knot)
{
    knots.push_back(new_knot);
}

void Spline::remove_knot(uint32_t index)
{
    knots.erase(knots.begin() + index);
}

void Spline::pop_knot()
{
    if (!knots.empty()) {
        knots.pop_back();
    }
}

void Spline::clear()
{
    knots.clear();
}

// Bezier splines

Knot BezierSpline::evaluate_quadratic(float t, const Knot& p0, const Knot& p1, const Knot& p2) const
{
    float u = 1.0f - t;
    return (u * u) * p0 + 2.0f * u * t * p1 + (t * t) * p2;
}

Knot BezierSpline::evaluate_cubic(float t, const Knot& p0, const Knot& p1, const Knot& p2, const Knot& p3) const
{
    float u = 1.0f - t;
    return (u * u * u) * p0 + 3.0f * (u * u) * t * p1 + 3.0f * u * (t * t) * p2 + (t * t * t) * p3;
}

void BezierSpline::compute_luts(uint32_t segments)
{
    luts.clear();
    luts.resize(segments);

    Knot start_point = knots.front();

    for (uint32_t i = 0u; i < segments; i++) {
        fill_lut(i, 8u, start_point);
    }
}

void BezierSpline::fill_lut(uint32_t idx, uint32_t segments, Knot& start_point)
{
    std::vector<float>& lut = luts[idx];
    size_t n = luts.size();

    // Segmentate the curve and fill the LUT with the aproximated lengths of the segments
    float step = 1.0f / segments;
    float current_length = 0.0f;
    const Knot& end_point = knots.at(idx + 1u);
    Knot prev = evaluate_cubic(0.0f, start_point, control_points[idx], control_points[n + idx], end_point);
    lut.resize(segments);

    lut[0u] = 0.0f;

    for (uint32_t i = 1u; i < segments; i++) {

        const Knot curr = evaluate_cubic(i * step, start_point, control_points[idx], control_points[n + idx], end_point);
        const Knot delta = prev - curr;

        current_length += glm::length(delta.position);

        lut[i] = current_length;

        prev = curr;
    }

    start_point = end_point;
}

float BezierSpline::sample_lut(uint32_t idx, float f)
{
    std::vector<float>& lut = luts[idx];
    float arc_length = lut.back(); // total arc length
    float distance = f * arc_length;
    size_t n = lut.size();

    if (distance > 0.0f && distance < arc_length) {
        for (size_t i = 0; i < n - 1; i++) {
            if (distance > lut[i] && distance < lut[i + 1]) {
                return remap_range(distance, lut[i], lut[i + 1], i / (n - 1.0f), (i + 1) / (n - 1.0f));
            }
        }
    }

    return distance / arc_length;
}

std::vector<Knot> BezierSpline::construct_target_vector(uint32_t n)
{
    std::vector<Knot> result;
    result.resize(n);

    result[0] = knots.at(0u) + 2.0f * knots.at(1u);

    for (uint32_t i = 1; i < n - 1; i++) {
        result[i] = (knots.at(i) * 2.0f + knots.at(i + 1u)) * 2.0f;
    }

    result[result.size() - 1u] = knots.at(n - 1u) * 8.0f + knots.at(n);

    return result;
}

std::vector<float> BezierSpline::construct_lower_diagonal_vector(uint32_t length)
{
    std::vector<float> result;
    result.resize(length);

    for (uint32_t i = 0u; i < result.size() - 1u; i++) {
        result[i] = 1.0f;
    }

    result[result.size() - 1u] = 2.0f;

    return result;
}

std::vector<float> BezierSpline::construct_main_diagonal_vector(uint32_t n)
{
    std::vector<float> result;
    result.resize(n);

    result[0] = 2.0f;

    for (uint32_t i = 1u; i < result.size() - 1u; i++) {
        result[i] = 4.0f;
    }

    result[result.size() - 1u] = 7.0f;

    return result;
}

std::vector<float> BezierSpline::construct_upper_diagonal_vector(uint32_t length)
{
    std::vector<float> result;
    result.resize(length);

    for (int i = 0u; i < result.size(); i++) {
        result[i] = 1.0f;
    }

    return result;
}

void BezierSpline::compute_control_points(uint32_t segments)
{
    std::vector<Knot>& result = control_points;
    result.clear();
    result.resize(2 * segments);

    std::vector<Knot> target = construct_target_vector(segments);
    std::vector<float> lower_diag = construct_lower_diagonal_vector(segments - 1);
    std::vector<float> main_diag = construct_main_diagonal_vector(segments);
    std::vector<float> upper_diag = construct_upper_diagonal_vector(segments - 1);

    std::vector<Knot> new_target;
    new_target.resize(segments);
    std::vector<float> new_upper_diag;
    new_upper_diag.resize(segments - 1u);

    // forward sweep for control points c_i,0:
    new_upper_diag[0] = upper_diag[0] / main_diag[0];
    new_target[0] = target[0] * (1.0f / main_diag[0]);

    for (uint32_t i = 1u; i < segments - 1u; i++) {
        new_upper_diag[i] = upper_diag[i] / (main_diag[i] - lower_diag[i - 1u] * new_upper_diag[i - 1u]);
    }

    for (uint32_t i = 1u; i < segments; i++) {
        float targetScale = 1.0f / (main_diag[i] - lower_diag[i - 1] * new_upper_diag[i - 1]);
        new_target[i] = (target[i] - new_target[i - 1u] * lower_diag[i - 1u]) * targetScale;
    }

    // backward sweep for control points c_i,0:
    result[segments - 1u] = new_target[segments - 1u];

    for (int i = segments - 2u; i >= 0; i--) {
        result[i] = new_target[i] - new_upper_diag[i] * result[i + 1u];
    }

    // calculate remaining control points c_i,1 directly:
    for (uint32_t i = 0u; i < segments - 1u; i++) {
        result[segments + i] = knots[i + 1u] * 2.0f - result[i + 1u];
    }

    result[2u * segments - 1u] = (knots[segments] + result[segments - 1u]) * 0.5f;
}

void BezierSpline::add_knot(const Knot& new_knot)
{
    knots.push_back(new_knot);

    size_t count = knots.size();
    if (count > 1) {
        for (uint32_t i = 0u; i < count - 1; ++i) {

            Knot& knot = knots[i];
            Knot& next_knot = knots[i + 1];

            glm::vec3 forward = glm::normalize(next_knot.position - knot.position);
            glm::vec3 reference_forward = glm::vec3(0.0f, 0.0f, -1.0f);
            // knot.rotation = glm::rotation(reference_forward, forward);
        }
    }

    dirty = true;
}

void BezierSpline::clear()
{
    Spline::clear();

    control_points.clear();
    luts.clear();
}

void BezierSpline::for_each(std::function<void(const Knot&)> fn)
{
    size_t n = knots.size();
    if (!n) {
        return;
    }

    uint32_t segments = static_cast<uint32_t>(n - 1);

    Knot start_point = knots.front();

    if (!segments) {
        fn(start_point);
    }

    else if (segments == 1u) {

        const Knot& end_point = knots[1];
        float distance = glm::distance(start_point.position, end_point.position);
        float k_distance = knot_distance;
        // Compute new knot distance based on density
        if (density != 0u) {
            k_distance = glm::max(k_distance, distance / static_cast<float>(density));
        }
        uint32_t number_of_edits = (uint32_t)glm::ceil(distance / k_distance);

        for (uint32_t j = 0; j < number_of_edits; ++j) {
            float t = static_cast<float>(j) / number_of_edits;
            const Knot& point = start_point * (1.0f - t) + end_point * t;
            fn(point);
        }
    }

    // Smooth path using bezier segments
    else {

        if (dirty) {
            compute_control_points(segments);
            compute_luts(segments);
            dirty = false;
        }

        float total_length = 0.0f;

        for (uint32_t i = 0; i < segments; i++) {
            const std::vector<float>& lut = luts[i];
            total_length += lut.back();
        }

        // Compute new knot distance based on density
        float k_distance = knot_distance;
        if (density != 0u) {
            k_distance = glm::max(k_distance, total_length / static_cast<float>(density));
        }

        for (uint32_t i = 0; i < segments; i++) {

            const std::vector<float>& lut = luts[i];
            uint32_t number_of_edits = (uint32_t)glm::ceil(lut.back() / k_distance);

            const Knot& end_point = knots.at(i + 1u);

            for (uint32_t j = 0; j < number_of_edits; ++j) {
                // Non-uniform space between knots..
                float t = static_cast<float>(j) / number_of_edits;
                // Use arc length approximation to compute the evenly distributed distances
                t = sample_lut(i, t);
                const Knot& point = evaluate_cubic(t, start_point, control_points[i], control_points[segments + i], end_point);
                fn(point);
            }

            start_point = end_point;
        }
    }
}

// B-splines

//float BSpline::get_basis(int i, int k, float t, const std::vector<float>& knots) const
//{
//    if (k == 1) {
//        return (knots[i] <= t && t < knots[i + 1]) ? 1.0f : 0.0f;
//    }
//    else {
//        float denom1 = knots[i + k - 1] - knots[i];
//        float denom2 = knots[i + k] - knots[i + 1];
//        float term1 = denom1 == 0 ? 0 : (t - knots[i]) / denom1 * get_basis(i, k - 1, t, knots);
//        float term2 = denom2 == 0 ? 0 : (knots[i + k] - t) / denom2 * get_basis(i + 1, k - 1, t, knots);
//        return term1 + term2;
//    }
//}
//
//glm::vec3 BSpline::evaluate(float t) const
//{
//    glm::vec3 point(0.0f);
//    int n = points.size();
//    for (int i = 0; i < n; ++i) {
//        float basis = get_basis(i, degree + 1, t, knots);
//        point += basis * points[i];
//    }
//    return point;
//}
//
//void BSpline::for_each(std::function<void(const glm::vec3&)> fn)
//{
//    if (count() < (degree + 1)) {
//        return;
//    }
//
//    float step = 1.f / (density - 1u);
//
//    for (int i = 0; i < density; ++i) {
//        float t = i * step;
//        glm::vec3 point = evaluate(t);
//        fn(point);
//    }
//}
//
//void BSpline::add_point(const glm::vec3& new_point)
//{
//    points.push_back(new_point);
//
//    // Knot vector (uniform in this case)
//    int control_points_count = points.size();
//    knots.clear();
//    knots.resize(control_points_count + degree + 1);
//
//    for (int i = 0; i < knots.size(); ++i) {
//        knots[i] = static_cast<float>(i) / (knots.size() - 1);
//    }
//}

// NURBS

//glm::vec3 NURBS::evaluate(float t) const
//{
//    t *= (knots.back() - knots.front()) + knots.front();
//
//    glm::vec3 numerator(0.0f);
//    float denominator = 0.0f;
//    int n = points.size();
//    for (int i = 0; i < n; ++i) {
//        float basis = get_basis(i, degree + 1, t, knots) * weights[i];
//        numerator += basis * points[i];
//        denominator += basis;
//    }
//    return numerator / denominator;
//}
//
//void NURBS::add_weighted_point(const glm::vec3& new_point, float weight)
//{
//    points.push_back(new_point);
//
//    weights.push_back(weight);
//
//    uint32_t d = degree + 1;
//    int control_points_count = points.size();
//    knots.clear();
//    knots.resize(control_points_count + d);
//
//    for (int i = 0; i < knots.size(); ++i) {
//        knots[i] = i < d ? 0.0f : (i >= (knots.size() - d) ? 1.0f : static_cast<float>(i) / (knots.size() - 1));
//    }
//}
