#include "spline.h"

// Splines

void Spline::add_point(const glm::vec3& new_point)
{
    points.push_back(new_point);
}

// Bezier splines

glm::vec3 BezierSpline::evaluate_quadratic(float t, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) const
{
    float u = 1.0f - t;
    return (u * u) * p0 + 2.0f * u * t * p1 + (t * t) * p2;
}

glm::vec3 BezierSpline::evaluate_cubic(float t, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) const
{
    float u = 1.0f - t;
    return (u * u * u) * p0 + 3.0f * (u * u) * t * p1 + 3.0f * u * (t * t) * p2 + (t * t * t) * p3;
}

std::vector<glm::vec3> BezierSpline::construct_target_vector(uint32_t n)
{
    std::vector<glm::vec3> result;
    result.resize(n);

    result[0] = points.at(0) + 2.0f * points.at(1);

    for (int i = 1; i < n - 1; i++) {
        result[i] = (points.at(i) * 2.0f + points.at(i + 1)) * 2.0f;
    }

    result[result.size() - 1] = points.at(n - 1) * 8.0f + points.at(n);

    return result;
}

std::vector<float> BezierSpline::construct_lower_diagonal_vector(uint32_t length)
{
    std::vector<float> result;
    result.resize(length);

    for (uint32_t i = 0; i < result.size() - 1; i++) {
        result[i] = 1.0f;
    }

    result[result.size() - 1] = 2.0f;

    return result;
}

std::vector<float> BezierSpline::construct_main_diagonal_vector(uint32_t n)
{
    std::vector<float> result;
    result.resize(n);

    result[0] = 2.0f;

    for (uint32_t i = 1; i < result.size() - 1; i++) {
        result[i] = 4.0f;
    }

    result[result.size() - 1] = 7.0f;

    return result;
}

std::vector<float> BezierSpline::construct_upper_diagonal_vector(uint32_t length)
{
    std::vector<float> result;
    result.resize(length);

    for (int i = 0; i < result.size(); i++) {
        result[i] = 1.0f;
    }

    return result;
}

std::vector<glm::vec3> BezierSpline::compute_control_points(uint32_t segments)
{
    std::vector<glm::vec3> result;
    result.resize(2 * segments);

    std::vector<glm::vec3> target = construct_target_vector(segments);
    std::vector<float> lowerDiag = construct_lower_diagonal_vector(segments - 1);
    std::vector<float> mainDiag = construct_main_diagonal_vector(segments);
    std::vector<float> upperDiag = construct_upper_diagonal_vector(segments - 1);

    std::vector<glm::vec3> new_target;
    new_target.resize(segments);
    std::vector<float> new_upper_diag;
    new_upper_diag.resize(segments - 1u);

    // forward sweep for control points c_i,0:
    new_upper_diag[0] = upperDiag[0] / mainDiag[0];
    new_target[0] = target[0] * (1.0f / mainDiag[0]);

    for (uint32_t i = 1; i < segments - 1u; i++) {
        new_upper_diag[i] = upperDiag[i] / (mainDiag[i] - lowerDiag[i - 1] * new_upper_diag[i - 1]);
    }

    for (uint32_t i = 1; i < segments; i++) {
        float targetScale = 1.0f / (mainDiag[i] - lowerDiag[i - 1] * new_upper_diag[i - 1]);
        new_target[i] = (target[i] - new_target[i - 1] * lowerDiag[i - 1]) * targetScale;
    }

    // backward sweep for control points c_i,0:
    result[segments - 1u] = new_target[segments - 1u];

    for (int i = segments - 2; i >= 0; i--) {
        result[i] = new_target[i] - new_upper_diag[i] * result[i + 1];
    }

    // calculate remaining control points c_i,1 directly:
    for (uint32_t i = 0; i < segments - 1u; i++) {
        result[segments + i] = points[i + 1] * 2.0f - result[i + 1];
    }

    result[2u * segments - 1u] = (points[segments] + result[segments - 1u]) * 0.5f;

    return result;
}

void BezierSpline::for_each(std::function<void(const glm::vec3&)> fn)
{
    int n = count();
    if (!n) {
        return;
    }

    uint32_t segments = points.size() - 1;

    glm::vec3 start_point = points[0];

    if (!segments) {
        fn(start_point);
    }

    else if (segments == 1) {
        for (int j = 0; j < density; ++j) {
            float t = static_cast<float>(j) / (density - 1);
            const glm::vec3& end_point = points[1];
            fn(glm::mix(start_point, end_point, t));
        }
    }

    // Smooth path using bezier segments
    else {
        const std::vector<glm::vec3>& control_points = compute_control_points(segments);

        for (uint32_t i = 0; i < segments; i++) {
            const glm::vec3& end_point = points.at(i + 1);

            for (int j = 0; j < density; ++j) {
                float t = static_cast<float>(j) / (density - 1u);
                const glm::vec3& point = evaluate_cubic(t, start_point, control_points[i], control_points[segments + i], end_point);
                fn(point);
            }

            start_point = end_point;
        }
    }
}

// B-splines

float BSpline::get_basis(int i, int k, float t, const std::vector<float>& knots) const
{
    if (k == 1) {
        return (knots[i] <= t && t < knots[i + 1]) ? 1.0f : 0.0f;
    }
    else {
        float denom1 = knots[i + k - 1] - knots[i];
        float denom2 = knots[i + k] - knots[i + 1];
        float term1 = denom1 == 0 ? 0 : (t - knots[i]) / denom1 * get_basis(i, k - 1, t, knots);
        float term2 = denom2 == 0 ? 0 : (knots[i + k] - t) / denom2 * get_basis(i + 1, k - 1, t, knots);
        return term1 + term2;
    }
}

glm::vec3 BSpline::evaluate(float t) const
{
    glm::vec3 point(0.0f);
    int n = points.size();
    for (int i = 0; i < n; ++i) {
        float basis = get_basis(i, degree + 1, t, knots);
        point += basis * points[i];
    }
    return point;
}

void BSpline::for_each(std::function<void(const glm::vec3&)> fn)
{
    if (count() < (degree + 1)) {
        return;
    }

    float step = 1.f / (density - 1u);

    for (int i = 0; i < density; ++i) {
        float t = i * step;
        glm::vec3 point = evaluate(t);
        fn(point);
    }
}

void BSpline::add_point(const glm::vec3& new_point)
{
    points.push_back(new_point);

    // Knot vector (uniform in this case)
    int control_points_count = points.size();
    knots.clear();
    knots.resize(control_points_count + degree + 1);

    for (int i = 0; i < knots.size(); ++i) {
        knots[i] = static_cast<float>(i) / (knots.size() - 1);
    }
}

// NURBS

glm::vec3 NURBS::evaluate(float t) const
{
    t *= (knots.back() - knots.front()) + knots.front();

    glm::vec3 numerator(0.0f);
    float denominator = 0.0f;
    int n = points.size();
    for (int i = 0; i < n; ++i) {
        float basis = get_basis(i, degree + 1, t, knots) * weights[i];
        numerator += basis * points[i];
        denominator += basis;
    }
    return numerator / denominator;
}

void NURBS::add_weighted_point(const glm::vec3& new_point, float weight)
{
    points.push_back(new_point);

    weights.push_back(weight);

    uint32_t d = degree + 1;
    int control_points_count = points.size();
    knots.clear();
    knots.resize(control_points_count + d);

    for (int i = 0; i < knots.size(); ++i) {
        knots[i] = i < d ? 0.0f : (i >= (knots.size() - d) ? 1.0f : static_cast<float>(i) / (knots.size() - 1));
    }
}
