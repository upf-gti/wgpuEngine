#include "parse_ply.h"

#include "framework/nodes/gs_node.h"
#include "framework/math/math_utils.h"

#include "happly.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

bool parse_ply(const char* ply_path, std::vector<Node*>& entities)
{
    GSNode* gs_node = new GSNode();

    happly::PLYData ply_file(ply_path);

    uint32_t splats_count = ply_file.getElement("vertex").count;

    gs_node->initialize(splats_count);
    gs_node->set_name("gs_node");

    glm::quat default_quad = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);

    gs_node->set_rotation(glm::rotate(default_quad, static_cast<float>(PI), glm::vec3(1.0, 0.0, 0.0)));

    std::vector<float> opacity_v = ply_file.getElement("vertex").getProperty<float>("opacity");
    std::vector<float> scale0 = ply_file.getElement("vertex").getProperty<float>("scale_0");
    std::vector<float> scale1 = ply_file.getElement("vertex").getProperty<float>("scale_1");
    std::vector<float> scale2 = ply_file.getElement("vertex").getProperty<float>("scale_2");
    std::vector<float> rot0 = ply_file.getElement("vertex").getProperty<float>("rot_0");
    std::vector<float> rot1 = ply_file.getElement("vertex").getProperty<float>("rot_1");
    std::vector<float> rot2 = ply_file.getElement("vertex").getProperty<float>("rot_2");
    std::vector<float> rot3 = ply_file.getElement("vertex").getProperty<float>("rot_3");
    std::vector<float> x = ply_file.getElement("vertex").getProperty<float>("x");
    std::vector<float> y = ply_file.getElement("vertex").getProperty<float>("y");
    std::vector<float> z = ply_file.getElement("vertex").getProperty<float>("z");
    std::vector<float> f_dc_0 = ply_file.getElement("vertex").getProperty<float>("f_dc_0");
    std::vector<float> f_dc_1 = ply_file.getElement("vertex").getProperty<float>("f_dc_1");
    std::vector<float> f_dc_2 = ply_file.getElement("vertex").getProperty<float>("f_dc_2");

    std::vector<glm::vec4> positions;
    std::vector<glm::vec4> colors;

    std::vector<glm::quat> rotations;
    std::vector<glm::vec4> scales;

    for (uint32_t i = 0; i < splats_count; ++i)
    {
        const double SH_C0 = 0.28209479177387814;

        glm::vec4 position = { x[i], y[i], z[i], 0.0f };
        float opacity = { (1.0f / (1.0f + exp(-opacity_v[i]))) };
        glm::vec4 color = { (0.5 + SH_C0 * f_dc_0[i]), (0.5 + SH_C0 * f_dc_1[i]), (0.5 + SH_C0 * f_dc_2[i]), opacity };

        glm::quat rotation = { rot1[i], rot2[i], rot3[i], rot0[i] };
        rotation = glm::normalize(rotation);
        glm::vec4 scale = { exp(scale0[i]), exp(scale1[i]), exp(scale2[i]), 0.0f };

        positions.push_back(position);
        colors.push_back(color);
        rotations.push_back(rotation);
        scales.push_back(scale);
    }

    gs_node->set_render_buffers(positions, colors);
    gs_node->set_covariance_buffers(rotations, scales);

    gs_node->calculate_covariance();

    entities.push_back(gs_node);

    return false;
}
