#include "mesh.h"
#include "utils.h"
#include "texture.h"
#include "shader.h"

WebGPUContext* Mesh::webgpu_context = nullptr;
Mesh* Mesh::quad_mesh = nullptr;

Mesh::~Mesh()
{
    if (vertices.empty()) return;

    vertices.clear();

    wgpuBufferDestroy(vertex_buffer);
}

void Mesh::create_vertex_buffer()
{
    vertex_buffer = webgpu_context->create_buffer(get_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, vertices.data(), "mesh_buffer");
}

WGPUBuffer& Mesh::get_vertex_buffer()
{
    return vertex_buffer;
}

std::vector<InterleavedData> Mesh::generate_quad(float w, float h, const glm::vec3& position, const glm::vec3& normal, const glm::vec3& color)
{
    InterleavedData points[4];

    glm::vec3 vX = get_perpendicular(normal);
    glm::vec3 vY = glm::cross(normal, vX);
    glm::vec3 delta1 = w * vX;
    glm::vec3 delta2 = h * vY;

    // Build one corner of the square
    glm::vec3 orig = -0.5f * w * vX + 0.5f * h * vY;
    uint32_t counter = 0;

    for (unsigned short i1 = 0; i1 <= 1; i1++)
    {
        for (unsigned short i2 = 0; i2 <= 1; i2++)
        {
            auto vtx = &points[counter++];
            vtx->position = position + 2.f * (orig + float(i1) * delta1 - float(i2) * delta2);
            vtx->normal = normal;
            vtx->uv = glm::vec2(i2, i1);
        }
    }

    std::vector<InterleavedData> vertices;
    vertices.resize(6);

    counter = 0;

    vertices[counter++] = points[2]; 
    vertices[counter++] = points[0];
    vertices[counter++] = points[1];

    vertices[counter++] = points[2];
    vertices[counter++] = points[1];
    vertices[counter++] = points[3];

    return vertices;
}

void Mesh::create_quad(float w, float h, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    vertices = generate_quad(w, h, glm::vec3(0.f), normals::pZ, color);

    spdlog::trace("Quad mesh created ({} vertices)", vertices.size());

    create_vertex_buffer();
}

void Mesh::create_box(float w, float h, float d, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    auto pos_x = generate_quad(w, h, d * normals::pX, normals::pX, color);
    vertices.insert(vertices.end(), pos_x.begin(), pos_x.end());
    auto neg_x = generate_quad(w, h, d * normals::nX, normals::nX, color);
    vertices.insert(vertices.end(), neg_x.begin(), neg_x.end());

    auto pos_y = generate_quad(w, h, d * normals::pY, normals::pY, color);
    vertices.insert(vertices.end(), pos_y.begin(), pos_y.end());
    auto neg_y = generate_quad(w, h, d * normals::nY, normals::nY, color);
    vertices.insert(vertices.end(), neg_y.begin(), neg_y.end());

    auto pos_z = generate_quad(w, h, d * normals::pZ, normals::pZ, color);
    vertices.insert(vertices.end(), pos_z.begin(), pos_z.end());
    auto neg_z = generate_quad(w, h, d * normals::nZ, normals::nZ, color);
    vertices.insert(vertices.end(), neg_z.begin(), neg_z.end());

    spdlog::trace("Box mesh created ({} vertices)", vertices.size());

    create_vertex_buffer();
}

void Mesh::create_rounded_box(float w, float h, float d, float c, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    w -= c;
    h -= c;

    // Add side vertices adding the chamfer translation
    auto pos_x = generate_quad(w, h, d * normals::pX, normals::pX, color);
    vertices.insert(vertices.end(), pos_x.begin(), pos_x.end());
    auto neg_x = generate_quad(w, h, d * normals::nX, normals::nX, color);
    vertices.insert(vertices.end(), neg_x.begin(), neg_x.end());

    auto pos_y = generate_quad(w, h, d * normals::pY, normals::pY, color);
    vertices.insert(vertices.end(), pos_y.begin(), pos_y.end());
    auto neg_y = generate_quad(w, h, d * normals::nY, normals::nY, color);
    vertices.insert(vertices.end(), neg_y.begin(), neg_y.end());

    auto pos_z = generate_quad(w, h, d * normals::pZ, normals::pZ, color);
    vertices.insert(vertices.end(), pos_z.begin(), pos_z.end());
    auto neg_z = generate_quad(w, h, d * normals::nZ, normals::nZ, color);
    vertices.insert(vertices.end(), neg_z.begin(), neg_z.end());

    constexpr float     pi = glm::pi<float>();
    constexpr float     half_pi = glm::pi<float>() * 0.5f;
    constexpr uint32_t  chamfer_seg = 8;

    // Substract now so it's only applied to corners and edges...
    d -= c;

    auto add_corner = [&](bool isXPositive, bool isYPositive, bool isZPositive)
        {
            std::vector<InterleavedData> vtxs;
            vtxs.resize(chamfer_seg * chamfer_seg * 6);
            uint32_t vtx_counter = 0;

            std::vector<uint32_t> indices;
            indices.resize(chamfer_seg * chamfer_seg * 6);
            uint32_t idx_counter = 0;

            int offset = 0;

            glm::vec3 offsetPosition = glm::vec3((isXPositive ? 1.f : -1.f) * w, (isYPositive ? 1.f : -1.f) * h, (isZPositive ? 1.f : -1.f) * d);
            float deltaRingAngle = (half_pi / chamfer_seg);
            float deltaSegAngle = (half_pi / chamfer_seg);
            float offsetRingAngle = isYPositive ? 0.f : half_pi;
            float offsetSegAngle;

            if (isXPositive && isZPositive) offsetSegAngle = 0;
            if ((!isXPositive) && isZPositive) offsetSegAngle = 1.5f * pi;
            if (isXPositive && (!isZPositive)) offsetSegAngle = half_pi;
            if ((!isXPositive) && (!isZPositive)) offsetSegAngle = pi;

            // Generate the group of rings for the sphere
            for (unsigned short ring = 0; ring <= chamfer_seg; ring++)
            {
                float r0 = c * sinf(ring * deltaRingAngle + offsetRingAngle);
                float y0 = c * cosf(ring * deltaRingAngle + offsetRingAngle);

                // Generate the group of segments for the current ring
                for (unsigned short seg = 0; seg <= chamfer_seg; seg++)
                {
                    float x0 = r0 * sinf(seg * deltaSegAngle + offsetSegAngle);
                    float z0 = r0 * cosf(seg * deltaSegAngle + offsetSegAngle);

                    // Store locally the different vertices
                    vtxs[vtx_counter++] = {
                        glm::vec3(x0 + offsetPosition.x, y0 + offsetPosition.y, z0 + offsetPosition.z),
                        glm::vec2((float)seg / (float)chamfer_seg, (float)ring / (float)chamfer_seg),
                        glm::normalize(glm::vec3(x0, y0, z0)),
                        color
                    };

                    if ((ring != chamfer_seg) && (seg != chamfer_seg))
                    {
                        // Each vertex (except the last) has six indices pointing to it
                        indices[idx_counter++] = (offset + chamfer_seg + 2);
                        indices[idx_counter++] = (offset);
                        indices[idx_counter++] = (offset + chamfer_seg + 1);
                        indices[idx_counter++] = (offset + chamfer_seg + 2);
                        indices[idx_counter++] = (offset + 1);
                        indices[idx_counter++] = (offset);
                    }

                    offset++;
                }
            }

            // Add vertices...
            uint32_t vertex_offset = vertices.size();
            vertices.resize(vertex_offset + indices.size());
            for (uint32_t i = 0; i < indices.size(); i++)
                vertices[vertex_offset + i] = vtxs[ indices[i] ];
        };

    // Add corners
    add_corner(true, true, true);       //  x,  y,  z
    add_corner(true, true, false);      //  x,  y, -z
    add_corner(true, false, true);      //  x, -y,  z
    add_corner(true, false, false);     //  x, -y, -z
    add_corner(false, true, true);      // -x,  y,  z
    add_corner(false, true, false);     // -x,  y, -z
    add_corner(false, false, true);     // -x, -y,  z
    add_corner(false, false, false);    // -x, -y, -z

    auto add_edge = [&](short xPos, short yPos, short zPos)
        {
            std::vector<InterleavedData> vtxs;
            vtxs.resize(chamfer_seg* chamfer_seg * 6);
            uint32_t vtx_counter = 0;

            std::vector<uint32_t> indices;
            indices.resize(chamfer_seg* chamfer_seg * 6);
            uint32_t idx_counter = 0;

            int offset = 0;

            glm::vec3 centerPosition = xPos * w * normals::pX + yPos * h * normals::pY + zPos * d * normals::pZ;
            glm::vec3 vy0 = (1.f - fabsf(xPos)) * normals::pX + (1.f - fabsf(yPos)) * normals::pY + (1.f - fabsf(zPos)) * normals::pZ;//extrusion direction

            glm::vec3 vx0 = glm::vec3(vy0.y, vy0.z, vy0.x) ;    // anti permute
            glm::vec3 vz0 = glm::vec3(vy0.z, vy0.x, vy0.y);     // permute

            if (glm::dot(vx0, centerPosition) < 0.f) vx0 = -vx0;
            if (glm::dot(vz0, centerPosition) < 0.f) vz0 = -vz0;
            if (glm::dot(glm::cross(vx0, vy0), vz0) < 0.f) vy0 = -vy0;

            float height = (1.f - fabsf(xPos)) * w + (1.f - fabsf(yPos)) * h + (1.f - fabsf(zPos)) * d;
            height *= 2.f;
            glm::vec3 offsetPosition = centerPosition - 0.5f * height * vy0;
            int numSegHeight = 1;

            float deltaAngle = (half_pi / chamfer_seg);
            float deltaHeight = height / (float)numSegHeight;

            for (unsigned short i = 0; i <= numSegHeight; i++)
            {
                for (unsigned short j = 0; j <= chamfer_seg; j++)
                {
                    float x0 = c * cosf(j * deltaAngle);
                    float z0 = c * sinf(j * deltaAngle);

                    // Store locally the different vertices
                    vtxs[vtx_counter++] = { glm::vec3(x0 * vx0 + i * deltaHeight * vy0 + z0 * vz0 + offsetPosition),
                        glm::vec2(j / (float)chamfer_seg, i / (float)numSegHeight),
                        glm::normalize(glm::vec3(x0 * vx0 + z0 * vz0)),
                        color
                    };

                    if (i != numSegHeight && j != chamfer_seg)
                    {
                        indices[idx_counter++] = (offset + chamfer_seg + 2);
                        indices[idx_counter++] = (offset);
                        indices[idx_counter++] = (offset + chamfer_seg + 1);
                        indices[idx_counter++] = (offset + chamfer_seg + 2);
                        indices[idx_counter++] = (offset + 1);
                        indices[idx_counter++] = (offset);
                    }

                    offset++;
                }
            }

            // Add vertices...
            uint32_t vertex_offset = vertices.size();
            vertices.resize(vertex_offset + indices.size());
            for (uint32_t i = 0; i < indices.size(); i++)
                vertices[vertex_offset + i] = vtxs[indices[i]];
        };

    // Generate the edges
    add_edge(-1, -1,  0);
    add_edge(-1,  1,  0);
    add_edge( 1, -1,  0);
    add_edge( 1,  1,  0);
    add_edge(-1,  0, -1);
    add_edge(-1,  0,  1);
    add_edge( 1,  0, -1);
    add_edge( 1,  0,  1);
    add_edge( 0, -1, -1);
    add_edge( 0, -1,  1);
    add_edge( 0,  1, -1);
    add_edge( 0,  1,  1);

    spdlog::trace("Rounded Box mesh created ({} vertices)", vertices.size());

    create_vertex_buffer();
}

void Mesh::create_sphere(float r, uint32_t segments, uint32_t rings, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    std::vector<InterleavedData> vtxs;
    vtxs.resize((rings + 1) * (segments + 1));
    uint32_t vtx_counter = 0;

    std::vector<uint32_t> indices;
    indices.resize(rings * (segments + 1) * 6);
    uint32_t idx_counter = 0;

    constexpr float pi = glm::pi<float>();
    constexpr float pi2 = pi * 2.f;

    float fDeltaRingAngle = (pi / rings);
    float fDeltaSegAngle = (pi2 / segments);
    int offset = 0;

    // Generate the group of rings for the sphere
    for (unsigned int ring = 0; ring <= rings; ring++)
    {
        float r0 = r * sinf(ring * fDeltaRingAngle);
        float y0 = r * cosf(ring * fDeltaRingAngle);

        // Generate the group of segments for the current ring
        for (unsigned int seg = 0; seg <= segments; seg++)
        {
            float x0 = r0 * sinf(seg * fDeltaSegAngle);
            float z0 = r0 * cosf(seg * fDeltaSegAngle);

            // Add one vertex to the strip which makes up the sphere
            vtxs[vtx_counter++] = { glm::vec3(x0, y0, z0),
                glm::vec2((float)seg / (float)segments, (float)ring / (float)rings),
                glm::normalize(glm::vec3(x0, y0, z0))
            };

            if (ring != rings)
            {
                if (seg != segments)
                {
                    // each vertex (except the last) has six indices pointing to it
                    if (ring != rings - 1)
                    {
                        indices[idx_counter++] = offset + segments + 2;
                        indices[idx_counter++] = offset;
                        indices[idx_counter++] = offset + segments + 1;
                    }
                    if (ring != 0)
                    {
                        indices[idx_counter++] = offset + segments + 2;
                        indices[idx_counter++] = offset + 1;
                        indices[idx_counter++] = offset;
                    }
                }
                offset++;
            }
        }
    }

    // Add vertices...
    vertices.resize(indices.size());
    for (uint32_t i = 0; i < indices.size(); i++)
        vertices[i] = vtxs[indices[i]];

    spdlog::trace("Sphere mesh created ({} vertices)", vertices.size());

    create_vertex_buffer();
}

void Mesh::create_cone(float r, float h, uint32_t segments, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    vertices.resize(segments * 6 * 2);

    constexpr float pi2 = glm::pi<float>() * 2.f;
    float deltaAngle = pi2 / float(segments);
    float normal_y = r / h;
    uint32_t vtx_counter = 0;

    auto add_vertex = [&](const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv) {
        auto vtx = &vertices[vtx_counter++];
        vtx->position = p;
        vtx->normal = n;
        vtx->uv = uv;
        };

    for (uint32_t i = 0; i < segments; i++)
    {
        float angle = i * deltaAngle;

        glm::vec3 n = glm::normalize(glm::vec3(sinf(angle + deltaAngle * 0.5f), normal_y, cosf(angle + deltaAngle * 0.5f)));
        add_vertex(glm::vec3(0.f, h, 0.f), n, glm::vec2(i / float(segments), 1.f));

        float nx = sinf(angle);
        float nz = cosf(angle);
        n = glm::normalize(glm::vec3(nx, normal_y, nz));
        add_vertex(glm::vec3(nx * r, 0.f, nz * r), n, glm::vec2(i / float(segments), 0.f));

        nx = sinf(angle + deltaAngle);
        nz = cosf(angle + deltaAngle);
        n = glm::normalize(glm::vec3(nx, normal_y, nz));
        add_vertex(glm::vec3(nx * r, 0.f, nz * r), n, glm::vec2((i + 1) / float(segments), 0.f));
    }

    // Caps

    glm::vec3 bottom_center = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 down = glm::vec3(0.f, -1.f, 0.f);

    for (uint32_t i = 0; i < segments; ++i)
    {
        float angle = i * deltaAngle;

        glm::vec3 uv = glm::vec3(sinf(angle), 0.f, cosf(angle));
        glm::vec3 uv2 = glm::vec3(sinf(angle + deltaAngle), 0.f, cosf(angle + deltaAngle));

        add_vertex(glm::vec3(uv2[0] * r, 0.f, uv2[2] * r), down, glm::vec2(uv2[0] * 0.5f + 0.5f, uv2[2] * 0.5f + 0.5f));
        add_vertex(glm::vec3(uv[0] * r, 0.f, uv[2] * r), down, glm::vec2(uv[0] * 0.5f + 0.5f, uv[2] * 0.5f + 0.5f));
        add_vertex(bottom_center, down, glm::vec2(0.5f));
    }

    spdlog::trace("Cone mesh created ({} vertices)", vtx_counter);

    create_vertex_buffer();
}

void Mesh::create_cylinder(float r, float h, uint32_t segments, bool capped, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    vertices.resize(segments * 6);

    constexpr float pi2 = glm::pi<float>() * 2.f;
    float deltaAngle = pi2 / float(segments);
    uint32_t vtx_counter = 0;

    auto add_vertex = [&](const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv) {
        auto vtx = &vertices[vtx_counter++];
        vtx->position = p;
        vtx->normal = n;
        vtx->uv = uv;
    };

    for (uint32_t i = 0; i < segments; i++)
    {
        float angle = i * deltaAngle;

        float x0 = sinf(angle);
        float z0 = cosf(angle);
        glm::vec3 n0 = glm::normalize(glm::vec3(x0, 0.f, z0));

        float x1 = sinf(angle + deltaAngle);
        float z1 = cosf(angle + deltaAngle);
        glm::vec3 n1 = glm::normalize(glm::vec3(x1, 0.f, z1));

        // First triangle
        add_vertex(glm::vec3(x0 * r, h *  0.5f, z0 * r), n0, glm::vec2(i / float(segments), 1.f));
        add_vertex(glm::vec3(x0 * r, h * -0.5f, z0 * r), n0, glm::vec2(i / float(segments), 0.f));
        add_vertex(glm::vec3(x1 * r, h * -0.5f, z1 * r), n1, glm::vec2((i + 1) / float(segments), 0.f));

        // Second triangle
        add_vertex(glm::vec3(x1 * r, h *  0.5f, z1 * r), n1, glm::vec2((i + 1) / float(segments), 1.f));
        add_vertex(glm::vec3(x0 * r, h *  0.5f, z0 * r), n0, glm::vec2(i / float(segments), 1.f));
        add_vertex(glm::vec3(x1 * r, h * -0.5f, z1 * r), n1, glm::vec2((i + 1) / float(segments), 0.f));
    }

    // Caps
    if (capped)
    {
        vertices.resize(segments * 6 * 2);

        glm::vec3 top_center = glm::vec3(0.f, h * 0.5f, 0.f);
        glm::vec3 bottom_center = glm::vec3(0.f, h * -0.5f, 0.f);
        glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
        glm::vec3 down = glm::vec3(0.f, -1.f, 0.f);

        for (uint32_t i = 0; i < segments; ++i)
        {
            float angle = i * deltaAngle;

            glm::vec3 uv = glm::vec3(sinf(angle), 0.f, cosf(angle));
            glm::vec3 uv2 = glm::vec3(sinf(angle + deltaAngle), 0.f, cosf(angle + deltaAngle));

            // Top
            add_vertex(glm::vec3(uv[0]  * r, h * 0.5f, uv[2]  * r), up, glm::vec2(-uv[0] * 0.5f + 0.5f, uv[2] * 0.5f + 0.5f));
            add_vertex(glm::vec3(uv2[0] * r, h * 0.5f, uv2[2] * r), up, glm::vec2(-uv2[0] * 0.5f + 0.5f, uv2[2] * 0.5f + 0.5f));
            add_vertex(top_center, up, glm::vec2(0.5f));

            // Bottom
            add_vertex(glm::vec3(uv2[0] * r, h * -0.5, uv2[2] * r), down, glm::vec2(uv2[0] * 0.5 + 0.5, uv2[2] * 0.5 + 0.5));
            add_vertex(glm::vec3(uv[0]  * r, h * -0.5, uv[2]  * r), down, glm::vec2(uv[0] * 0.5 + 0.5, uv[2] * 0.5 + 0.5));
            add_vertex(bottom_center, down, glm::vec2(0.5f));
        }
    }

    spdlog::trace("Cylinder mesh created ({} vertices)", vtx_counter);

    create_vertex_buffer();
}

void Mesh::create_capsule(float r, float h, uint32_t segments, uint32_t rings, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    constexpr float pi = glm::pi<float>() * 2.f;
    constexpr float half_pi = glm::pi<float>() * 0.5f;
    constexpr float pi2 = glm::pi<float>() * 2.f;

    float delta_ring_angle = (half_pi / rings);
    float delta_seg_angle = (pi2 / segments);

    float sphere_ratio = r / (2 * r + h);
    float cylinder_ratio = h / (2 * r + h);

    uint32_t num_height_seg = 2;

    std::vector<InterleavedData> vtxs;
    const int num_vertices = (2 * rings + 2) * (segments + 1) + (num_height_seg - 1) * (segments + 1);
    vtxs.resize(num_vertices);
    uint32_t vtx_counter = 0;

    std::vector<uint32_t> indices;
    const int num_indices = (2 * rings + 1) * (segments + 1) * 6 + (num_height_seg - 1) * (segments + 1) * 6;
    indices.resize(num_indices);
    vertices.resize(num_indices);

    uint32_t idx_counter = 0;
    int offset = 0;

    // Top half sphere
    // Generate the group of rings for the sphere

    for (unsigned int ring = 0; ring <= rings; ring++)
    {
        float r0 = r * sinf(ring * delta_ring_angle);
        float y0 = r * cosf(ring * delta_ring_angle);

        // Generate the group of segments for the current ring
        for (unsigned int seg = 0; seg <= segments; seg++)
        {
            float x0 = r0 * cosf(seg * delta_seg_angle);
            float z0 = r0 * sinf(seg * delta_seg_angle);

            // Add one vertex to the strip which makes up the sphere
            vtxs[vtx_counter++] = { glm::vec3(x0, 0.5f * h + y0, z0),
                glm::vec2((float)seg / (float)segments, (float)ring / (float)rings * sphere_ratio),
                glm::normalize(glm::vec3(x0, y0, z0))
            };

            // each vertex (except the last) has six indices pointing to it
            indices[idx_counter++] = (offset + segments + 1);
            indices[idx_counter++] = (offset + segments);
            indices[idx_counter++] = (offset);
            indices[idx_counter++] = (offset + segments + 1);
            indices[idx_counter++] = (offset);
            indices[idx_counter++] = (offset + 1);

            offset++;
        }
    }

    // Cylinder part

    float deltaAngle = (pi2 / segments);
    float deltah = h / float(num_height_seg);

    for (unsigned short i = 1; i < num_height_seg; i++)
        for (unsigned short j = 0; j <= segments; j++)
        {
            float x0 = r * cosf(j * deltaAngle);
            float z0 = r * sinf(j * deltaAngle);

            vtxs[vtx_counter++] = { glm::vec3(x0, 0.5f * h - i * deltah, z0),
                glm::vec2(j / (float)segments, (i / float(num_height_seg)) * cylinder_ratio + sphere_ratio),
                glm::normalize(glm::vec3(x0, 0, z0))
            };

            indices[idx_counter++] = (offset + segments + 1);
            indices[idx_counter++] = (offset + segments);
            indices[idx_counter++] = (offset);
            indices[idx_counter++] = (offset + segments + 1);
            indices[idx_counter++] = (offset);
            indices[idx_counter++] = (offset + 1);

            offset++;
        }

    // Bottom half sphere
    // Generate the group of rings for the sphere

    for (unsigned int ring = 0; ring <= rings; ring++)
    {
        float r0 = r * sinf(half_pi + ring * delta_ring_angle);
        float y0 = r * cosf(half_pi + ring * delta_ring_angle);

        // Generate the group of segments for the current ring
        for (unsigned int seg = 0; seg <= segments; seg++)
        {
            float x0 = r0 * cosf(seg * delta_seg_angle);
            float z0 = r0 * sinf(seg * delta_seg_angle);

            // Add one vertex to the strip which makes up the sphere
            vtxs[vtx_counter++] = { glm::vec3(x0, -0.5f * h + y0, z0),
                glm::vec2((float)seg / (float)segments, (float)ring / (float)rings * sphere_ratio + cylinder_ratio + sphere_ratio),
                glm::normalize(glm::vec3(x0, y0, z0))
            };

            if (ring != rings)
            {
                // each vertex (except the last) has six indices pointing to it
                indices[idx_counter++] = (offset + segments + 1);
                indices[idx_counter++] = (offset + segments);
                indices[idx_counter++] = (offset);
                indices[idx_counter++] = (offset + segments + 1);
                indices[idx_counter++] = (offset);
                indices[idx_counter++] = (offset + 1);
            }
            offset++;
        }
    }

    // Add vertices...
    for (uint32_t i = 0; i < indices.size(); i++)
        vertices[i] = vtxs[indices[i]];

    spdlog::trace("Capsule mesh created ({} vertices)", vertices.size());

    create_vertex_buffer();
}

void Mesh::create_torus(float r, float ir, uint32_t segments_section, uint32_t segments_circle, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        vertices.clear();
        wgpuBufferDestroy(vertex_buffer);
    }

    std::vector<InterleavedData> vtxs;
    vtxs.resize((segments_circle + 1) * (segments_section + 1));
    uint32_t vtx_counter = 0;

    std::vector<uint32_t> indices;
    indices.resize((segments_circle) * (segments_section + 1) * 6);
    uint32_t idx_counter = 0;

    constexpr float pi2 = glm::pi<float>() * 2.f;

    float deltaSection = (pi2 / segments_section);
    float deltaCircle = (pi2 / segments_circle);
    int offset = 0;

    for (unsigned int i = 0; i <= segments_circle; i++)
    {
        for (unsigned int j = 0; j <= segments_section; j++)
        {
            glm::vec3 c0(r, 0.0, 0.0);
            glm::vec3 v0(r + ir * cosf(j * deltaSection), ir * sinf(j * deltaSection), 0.0);
            glm::quat q = glm::angleAxis(i * deltaCircle, normals::pY);
            glm::vec3 v = q * v0;
            glm::vec3 c = q * c0;

            vtxs[ vtx_counter++ ] = { v,
                glm::vec2(i / (float)segments_circle, j / (float)segments_section),
                glm::normalize(v - c) };

            if (i != segments_circle)
            {
                indices[idx_counter++] = offset + segments_section + 1;
                indices[idx_counter++] = offset;
                indices[idx_counter++] = offset + segments_section;
                indices[idx_counter++] = offset + segments_section + 1;
                indices[idx_counter++] = offset + 1;
                indices[idx_counter++] = offset;
            }
            offset++;
        }
    }

    // Add vertices...
    vertices.resize(indices.size());
    for (uint32_t i = 0; i < indices.size(); i++)
        vertices[i] = vtxs[indices[i]];

    spdlog::trace("Torus mesh created ({} vertices)", vertices.size());

    create_vertex_buffer();
}

void Mesh::create_from_vertices(const std::vector<InterleavedData>& _vertices)
{
    vertices = _vertices;
    create_vertex_buffer();
}

void* Mesh::data()
{
    return vertices.data();
}

uint32_t Mesh::get_vertex_count()
{
    return static_cast<uint32_t>(vertices.size());
}

uint64_t Mesh::get_byte_size()
{
    return get_vertex_count() * sizeof(InterleavedData);
}
