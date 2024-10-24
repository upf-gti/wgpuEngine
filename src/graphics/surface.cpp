#include "surface.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"

WebGPUContext* Surface::webgpu_context = nullptr;
Surface* Surface::quad_mesh = nullptr;

Surface::~Surface()
{
    if (vertex_buffer) {
        wgpuBufferDestroy(vertex_buffer);
    }

    if (material) {
        if (material->unref()) {
            RendererStorage::delete_material_bind_group(webgpu_context, material);
        }
    }
}

void Surface::set_material(Material* material)
{
    if (this->material != material) {

        if (this->material) {
            this->material->unref();
        }

        material->ref();
    }

    this->material = material;
}

void Surface::update_vertex_buffer(const std::vector<sInterleavedData>& _vertices)
{
    if (_vertices.size() > vertex_count && vertex_buffer) {
        wgpuBufferDestroy(vertex_buffer);
        vertex_buffer = nullptr;
    }

    if (!vertex_buffer) {
        create_from_vertices(_vertices);
    }

    vertex_count = _vertices.size();
    webgpu_context->update_buffer(vertex_buffer, 0, _vertices.data(), get_vertices_byte_size());
}

void Surface::create_vertex_buffer(const std::vector<sInterleavedData>& vertices)
{
    vertex_count = vertices.size();
    vertex_buffer = webgpu_context->create_buffer(get_vertices_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, vertices.data(), ("mesh_buffer_" + name).c_str());
}

void Surface::create_index_buffer(const std::vector<uint32_t>& indices)
{
    index_count = indices.size();
    index_buffer = webgpu_context->create_buffer(get_indices_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index, indices.data(), ("index_buffer_" + name).c_str());
}

Material* Surface::get_material()
{
    return material;
}

const Material* Surface::get_material() const
{
    return material;
}

void Surface::set_surface_data(sSurfaceData* surface_data)
{
    this->surface_data = surface_data;
}

const sSurfaceData* Surface::get_surface_data() const
{
    return surface_data;
}

sSurfaceData* Surface::get_surface_data()
{
    return surface_data;
}

const WGPUBuffer& Surface::get_vertex_buffer() const
{
    return vertex_buffer;
}

const WGPUBuffer& Surface::get_index_buffer() const
{
    return index_buffer;
}

void Surface::create_axis(float s)
{
    std::vector<sInterleavedData> vertices;

    vertices.push_back({ glm::vec3(-1.0f,  0.0f,  0.0f) * s });
    vertices.push_back({ glm::vec3( 1.0f,  0.0f,  0.0f) * s });
    vertices.push_back({ glm::vec3( 0.0f, -1.0f,  0.0f) * s });
    vertices.push_back({ glm::vec3( 0.0f,  1.0f,  0.0f) * s });
    vertices.push_back({ glm::vec3( 0.0f,  0.0f, -1.0f) * s });
    vertices.push_back({ glm::vec3( 0.0f,  0.0f,  1.0f) * s });

    create_vertex_buffer(vertices);
}

std::vector<sInterleavedData> Surface::generate_quad(float w, float h, const glm::vec3& position, const glm::vec3& normal, const glm::vec3& color, bool flip_y)
{
    sInterleavedData points[4];

    glm::vec3 n = normal;
    glm::vec3 vY = get_perpendicular(n);
    glm::vec3 vX = glm::cross(n, vY);
    glm::vec3 delta1 = w * vX;
    glm::vec3 delta2 = h * vY;

    // Build one corner of the square
    glm::vec3 orig = -0.5f * w * vX - 0.5f * h * vY;
    uint32_t counter = 0;

    for (unsigned short i1 = 0; i1 <= 1; i1++)
    {
        for (unsigned short i2 = 0; i2 <= 1; i2++)
        {
            auto vtx = &points[counter++];
            vtx->position = position - (orig + float(i1) * delta1 + float(i2) * delta2);
            vtx->normal = n;
            vtx->uv = glm::vec2(i1, flip_y ? (1.0f - i2) : i2);
            vtx->color = color;
        }
    }

    std::vector<sInterleavedData> vertices;
    vertices.resize(6);

    counter = 0;

    if (flip_y) {
        vertices[counter++] = points[2];
        vertices[counter++] = points[1];
        vertices[counter++] = points[0];

        vertices[counter++] = points[2];
        vertices[counter++] = points[3];
        vertices[counter++] = points[1];
    }
    else {
        vertices[counter++] = points[0];
        vertices[counter++] = points[1];
        vertices[counter++] = points[2];

        vertices[counter++] = points[1];
        vertices[counter++] = points[3];
        vertices[counter++] = points[2];
    }

    return vertices;
}

void Surface::create_quad(float w, float h, bool flip_y, bool centered, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        wgpuBufferDestroy(vertex_buffer);
    }

    name = "quad";

    glm::vec3 origin(0.0f);

    if (!centered) {
        origin += glm::vec3(w * 0.5f, h * 0.5f, 0.0f);
    }

    std::vector<sInterleavedData> vertices = generate_quad(w, h, origin, normals::pZ, color, flip_y);

    spdlog::trace("Quad mesh created ({} vertices)", vertices.size());

    create_vertex_buffer(vertices);
}

void Surface::create_subdivided_quad(float w, float h, uint32_t subdivisions, bool centered, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer) {
        wgpuBufferDestroy(vertex_buffer);
    }

    name = "subdiv_quad";

    std::vector<sInterleavedData> vertices;

    float step_x = w / subdivisions;
    float step_y = h / subdivisions;

    glm::vec3 origin = { -w * 0.5f, h * 0.5f, 0.0f };

    if (!centered) {
        origin += glm::vec3(w * 0.5f, h * 0.5f, 0.0f);
    }

    // Generate vertices with positions and UVs
    for (int i = 0; i < subdivisions; ++i) {
        for (int j = 0; j < subdivisions; ++j) {

            const glm::vec3& new_origin = origin + glm::vec3(j * step_x, -i * step_y, 0.0f) + glm::vec3(step_x * 0.5f, -step_y * 0.5f, 0.0f);
            std::vector<sInterleavedData> vtxs = generate_quad(step_x, step_y, new_origin, normals::pZ, color);

            const glm::vec2& uv0 = { static_cast<float>(j) / subdivisions, static_cast<float>(i) / subdivisions };
            const glm::vec2& uv1 = { static_cast<float>(j + 1u) / subdivisions, static_cast<float>(i + 1u) / subdivisions };

            vtxs[0].uv = glm::vec2(uv1.x, 1.0f - uv0.y);
            vtxs[1].uv = glm::vec2(uv0.x, 1.0f - uv1.y);
            vtxs[2].uv = glm::vec2(uv0.x, 1.0f - uv0.y);

            vtxs[3].uv = glm::vec2(uv1.x, 1.0f - uv0.y);
            vtxs[4].uv = glm::vec2(uv1.x, 1.0f - uv1.y);;
            vtxs[5].uv = glm::vec2(uv0.x, 1.0f - uv1.y);

            vertices.insert(vertices.end(), vtxs.begin(), vtxs.end());
        }
    }

    spdlog::trace("Subvidided Quad mesh created ({} vertices)", vertices.size());

    create_vertex_buffer(vertices);
}

void Surface::create_box(float w, float h, float d, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer) {
        wgpuBufferDestroy(vertex_buffer);
    }

    name = "box";

    std::vector<sInterleavedData> vertices;

    float w2 = w * 2.0f;
    float h2 = h * 2.0f;
    float d2 = d * 2.0f;

    auto pos_x = generate_quad(h2, w2, d * normals::pX, -normals::pX, color, true);
    vertices.insert(vertices.end(), pos_x.begin(), pos_x.end());
    auto neg_x = generate_quad(h2, w2, d * normals::nX, -normals::nX, color, true);
    vertices.insert(vertices.end(), neg_x.begin(), neg_x.end());

    auto pos_y = generate_quad(w2, d2, h * normals::pY, -normals::pY, color, true);
    vertices.insert(vertices.end(), pos_y.begin(), pos_y.end());
    auto neg_y = generate_quad(w2, d2, h * normals::nY, -normals::nY, color, true);
    vertices.insert(vertices.end(), neg_y.begin(), neg_y.end());

    auto pos_z = generate_quad(w2, h2, d * normals::pZ, -normals::pZ, color, true);
    vertices.insert(vertices.end(), pos_z.begin(), pos_z.end());
    auto neg_z = generate_quad(w2, h2, d * normals::nZ, -normals::nZ, color, true);
    vertices.insert(vertices.end(), neg_z.begin(), neg_z.end());

    spdlog::trace("Box mesh created ({} vertices)", vertices.size());

    create_vertex_buffer(vertices);
}

void Surface::create_rounded_box(float w, float h, float d, float c, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer) {
        wgpuBufferDestroy(vertex_buffer);
    }

    name = "rounded_box";

    std::vector<sInterleavedData> vertices;

    w -= c;
    h -= c;

    float w2 = w * 2.0f;
    float h2 = h * 2.0f;
    float d2 = (d - c) * 2.0f;

    // Add side vertices adding the chamfer translation
    auto pos_x = generate_quad(h2, w2, d * normals::pX, -normals::pX, color, true);
    vertices.insert(vertices.end(), pos_x.begin(), pos_x.end());
    auto neg_x = generate_quad(h2, w2, d * normals::nX, -normals::nX, color, true);
    vertices.insert(vertices.end(), neg_x.begin(), neg_x.end());

    auto pos_y = generate_quad(w2, d2, (h + c) * normals::pY, -normals::pY, color, true);
    vertices.insert(vertices.end(), pos_y.begin(), pos_y.end());
    auto neg_y = generate_quad(w2, d2, (h + c) * normals::nY, -normals::nY, color, true);
    vertices.insert(vertices.end(), neg_y.begin(), neg_y.end());

    auto pos_z = generate_quad(w2, h2, d * normals::pZ, -normals::pZ, color, true);
    vertices.insert(vertices.end(), pos_z.begin(), pos_z.end());
    auto neg_z = generate_quad(w2, h2, d * normals::nZ, -normals::nZ, color, true);
    vertices.insert(vertices.end(), neg_z.begin(), neg_z.end());

    if (c > 0.0f) {

        float     pi = glm::pi<float>();
        float     half_pi = glm::pi<float>() * 0.5f;
        constexpr uint32_t  chamfer_seg = 8;

        d -= c;

        auto add_corner = [&](bool isXPositive, bool isYPositive, bool isZPositive)
        {
            std::vector<sInterleavedData> vtxs;
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

            if (isXPositive && isZPositive) offsetSegAngle = 0.0f;
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
                        glm::vec4(0.0f),
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
            size_t vertex_offset = vertices.size();
            vertices.resize(vertex_offset + indices.size());
            for (size_t i = 0; i < indices.size(); i++)
                vertices[vertex_offset + i] = vtxs[indices[i]];
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
            std::vector<sInterleavedData> vtxs;
            vtxs.resize(chamfer_seg * chamfer_seg * 6);
            uint32_t vtx_counter = 0;

            std::vector<uint32_t> indices;
            indices.resize(chamfer_seg * chamfer_seg * 6);
            uint32_t idx_counter = 0;

            int offset = 0;

            glm::vec3 centerPosition = xPos * w * normals::pX + yPos * h * normals::pY + zPos * d * normals::pZ;
            glm::vec3 vy0 = (1.f - std::abs(xPos)) * normals::pX + (1.f - std::abs(yPos)) * normals::pY + (1.f - std::abs(zPos)) * normals::pZ;//extrusion direction

            glm::vec3 vx0 = glm::vec3(vy0.y, vy0.z, vy0.x);    // anti permute
            glm::vec3 vz0 = glm::vec3(vy0.z, vy0.x, vy0.y);     // permute

            if (glm::dot(vx0, centerPosition) < 0.f) vx0 = -vx0;
            if (glm::dot(vz0, centerPosition) < 0.f) vz0 = -vz0;
            if (glm::dot(glm::cross(vx0, vy0), vz0) < 0.f) vy0 = -vy0;

            float height = (1.f - std::abs(xPos)) * w + (1.f - std::abs(yPos)) * h + (1.f - std::abs(zPos)) * d;
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
                        glm::vec4(0.0f),
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
            size_t vertex_offset = vertices.size();
            vertices.resize(vertex_offset + indices.size());
            for (size_t i = 0; i < indices.size(); i++)
                vertices[vertex_offset + i] = vtxs[indices[i]];
        };

        // Generate the edges
        add_edge(-1, -1, 0);
        add_edge(-1, 1, 0);
        add_edge(1, -1, 0);
        add_edge(1, 1, 0);
        add_edge(-1, 0, -1);
        add_edge(-1, 0, 1);
        add_edge(1, 0, -1);
        add_edge(1, 0, 1);
        add_edge(0, -1, -1);
        add_edge(0, -1, 1);
        add_edge(0, 1, -1);
        add_edge(0, 1, 1);
    }

    spdlog::trace("Rounded Box mesh created ({} vertices)", vertices.size());

    create_vertex_buffer(vertices);
}

void Surface::create_sphere(float r, uint32_t segments, uint32_t rings, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        wgpuBufferDestroy(vertex_buffer);
    }

    name = "sphere";

    std::vector<sInterleavedData> vertices;

    std::vector<sInterleavedData> vtxs;
    vtxs.resize((rings + 1) * (segments + 1));
    uint32_t vtx_counter = 0;

    std::vector<uint32_t> indices;
    indices.resize(rings * (segments + 1) * 6);
    uint32_t idx_counter = 0;

    float pi = glm::pi<float>();
    float pi2 = pi * 2.f;

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

    create_vertex_buffer(vertices);
}

void Surface::create_cone(float r, float h, uint32_t segments, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        wgpuBufferDestroy(vertex_buffer);
    }

    name = "cone";

    std::vector<sInterleavedData> vertices;

    vertices.resize(segments * 6 * 2);

    float pi2 = glm::pi<float>() * 2.f;
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

    create_vertex_buffer(vertices);
}

void Surface::create_cylinder(float r, float h, uint32_t segments, bool capped, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        wgpuBufferDestroy(vertex_buffer);
    }

    name = "cylinder";

    std::vector<sInterleavedData> vertices;

    vertices.resize(segments * 6);

    float pi2 = glm::pi<float>() * 2.f;
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
        add_vertex(glm::vec3(x0 * r, h * 0.5f, z0 * r), n0, glm::vec2(i / float(segments), 1.f));
        add_vertex(glm::vec3(x0 * r, h * -0.5f, z0 * r), n0, glm::vec2(i / float(segments), 0.f));
        add_vertex(glm::vec3(x1 * r, h * -0.5f, z1 * r), n1, glm::vec2((i + 1) / float(segments), 0.f));

        // Second triangle
        add_vertex(glm::vec3(x1 * r, h * 0.5f, z1 * r), n1, glm::vec2((i + 1) / float(segments), 1.f));
        add_vertex(glm::vec3(x0 * r, h * 0.5f, z0 * r), n0, glm::vec2(i / float(segments), 1.f));
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
            add_vertex(glm::vec3(uv[0] * r, h * 0.5f, uv[2] * r), up, glm::vec2(-uv[0] * 0.5f + 0.5f, uv[2] * 0.5f + 0.5f));
            add_vertex(glm::vec3(uv2[0] * r, h * 0.5f, uv2[2] * r), up, glm::vec2(-uv2[0] * 0.5f + 0.5f, uv2[2] * 0.5f + 0.5f));
            add_vertex(top_center, up, glm::vec2(0.5f));

            // Bottom
            add_vertex(glm::vec3(uv2[0] * r, h * -0.5, uv2[2] * r), down, glm::vec2(uv2[0] * 0.5 + 0.5, uv2[2] * 0.5 + 0.5));
            add_vertex(glm::vec3(uv[0] * r, h * -0.5, uv[2] * r), down, glm::vec2(uv[0] * 0.5 + 0.5, uv[2] * 0.5 + 0.5));
            add_vertex(bottom_center, down, glm::vec2(0.5f));
        }
    }

    spdlog::trace("Cylinder mesh created ({} vertices)", vtx_counter);

    create_vertex_buffer(vertices);
}

void Surface::create_capsule(float r, float h, uint32_t segments, uint32_t rings, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        wgpuBufferDestroy(vertex_buffer);
    }

    name = "capsule";

    std::vector<sInterleavedData> vertices;

    float pi = glm::pi<float>() * 2.f;
    float half_pi = glm::pi<float>() * 0.5f;
    float pi2 = glm::pi<float>() * 2.f;

    float delta_ring_angle = (half_pi / rings);
    float delta_seg_angle = (pi2 / segments);

    float sphere_ratio = r / (2 * r + h);
    float cylinder_ratio = h / (2 * r + h);

    uint32_t num_height_seg = 2;

    std::vector<sInterleavedData> vtxs;
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

    create_vertex_buffer(vertices);
}

void Surface::create_torus(float r, float ir, uint32_t segments_section, uint32_t segments_circle, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_buffer)
    {
        wgpuBufferDestroy(vertex_buffer);
    }

    name = "torus";

    std::vector<sInterleavedData> vertices;

    std::vector<sInterleavedData> vtxs;
    vtxs.resize((segments_circle + 1) * (segments_section + 1));
    uint32_t vtx_counter = 0;

    std::vector<uint32_t> indices;
    indices.resize((segments_circle) * (segments_section + 1) * 6);
    uint32_t idx_counter = 0;

    float pi2 = glm::pi<float>() * 2.f;

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

            vtxs[vtx_counter++] = { v,
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

    create_vertex_buffer(vertices);
}

void Surface::create_circle(float radius, uint32_t segments)
{
    name = "circle";

    std::vector<sInterleavedData> vertices;

    float increment = 2.0f * PI / segments;

    for (float currAngle = 0.0f; currAngle <= 2.0f * PI + increment; currAngle += increment)
    {
        vertices.push_back({ .position = glm::vec3(radius * cos(currAngle), radius * sin(currAngle), 0) });
    }

    spdlog::trace("Circle mesh created ({} vertices)", vertices.size());

    create_vertex_buffer(vertices);
}

void Surface::create_arrow()
{
    name = "arrow";

    std::vector<sInterleavedData> vertices;

    vertices.push_back({ .position = glm::vec3(1.0f, 0.0f, 0.0f) });
    vertices.push_back({ .position = glm::vec3(0.6f, 0.25f, 0.0f) });
    vertices.push_back({ .position = glm::vec3(0.6f, 0.1f, 0.0f) });
    vertices.push_back({ .position = glm::vec3(0.0f, 0.1f, 0.0f) });
    vertices.push_back({ .position = glm::vec3(0.0f, -0.1f, 0.0f) });
    vertices.push_back({ .position = glm::vec3(0.6f, -0.1f, 0.0f) });
    vertices.push_back({ .position = glm::vec3(0.6f, -0.25f, 0.0f) });
    vertices.push_back({ .position = glm::vec3(1.0f, 0.0f, 0.0f) });

    spdlog::trace("Circle mesh created ({} vertices)", vertices.size());

    create_vertex_buffer(vertices);
}

void Surface::create_skybox()
{
    name = "skybox";

    std::vector<sInterleavedData> vertices;

    vertices.resize(36);

    vertices[0].position = { -1.0f, 1.0f, -1.0f };
    vertices[1].position = { -1.0f, -1.0f, -1.0f };
    vertices[2].position = {  1.0f, -1.0f, -1.0f };
    vertices[3].position = {  1.0f, -1.0f, -1.0f };
    vertices[4].position = {  1.0f,  1.0f, -1.0f };
    vertices[5].position = { -1.0f,  1.0f, -1.0f };

    vertices[6].position = { -1.0f, -1.0f,  1.0f };
    vertices[7].position = { -1.0f, -1.0f, -1.0f };
    vertices[8].position = { -1.0f,  1.0f, -1.0f };
    vertices[9].position = { -1.0f,  1.0f, -1.0f };
    vertices[10].position = { -1.0f,  1.0f,  1.0f };
    vertices[11].position = { -1.0f, -1.0f,  1.0f };

    vertices[12].position = { 1.0f, -1.0f, -1.0f };
    vertices[13].position = { 1.0f, -1.0f,  1.0f };
    vertices[14].position = { 1.0f,  1.0f,  1.0f };
    vertices[15].position = { 1.0f,  1.0f,  1.0f };
    vertices[16].position = { 1.0f,  1.0f, -1.0f };
    vertices[17].position = { 1.0f, -1.0f, -1.0f };

    vertices[18].position = { -1.0f, -1.0f,  1.0f };
    vertices[19].position = { -1.0f,  1.0f,  1.0f };
    vertices[20].position = {  1.0f,  1.0f,  1.0f };
    vertices[21].position = {  1.0f,  1.0f,  1.0f };
    vertices[22].position = {  1.0f, -1.0f,  1.0f };
    vertices[23].position = { -1.0f, -1.0f,  1.0f };

    vertices[24].position = { -1.0f,  1.0f, -1.0f };
    vertices[25].position = { 1.0f,  1.0f, -1.0f };
    vertices[26].position = { 1.0f,  1.0f,  1.0f };
    vertices[27].position = { 1.0f,  1.0f,  1.0f };
    vertices[28].position = { -1.0f,  1.0f,  1.0f };
    vertices[29].position = { -1.0f,  1.0f, -1.0f };

    vertices[30].position = { -1.0f, -1.0f, -1.0f };
    vertices[31].position = { -1.0f, -1.0f,  1.0f };
    vertices[32].position = {  1.0f, -1.0f, -1.0f };
    vertices[33].position = {  1.0f, -1.0f, -1.0f };
    vertices[34].position = { -1.0f, -1.0f,  1.0f };
    vertices[35].position = { 1.0f, -1.0f,  1.0f };

    create_vertex_buffer(vertices);
}

void Surface::create_from_vertices(const std::vector<sInterleavedData>& _vertices)
{
    create_vertex_buffer(_vertices);
}
//
//void* Surface::data()
//{
//    return vertices.data();
//}

uint32_t Surface::get_vertex_count() const
{
    return vertex_count;
}

uint64_t Surface::get_vertices_byte_size() const
{
    return vertex_count * sizeof(sInterleavedData);
}

uint32_t Surface::get_index_count() const
{
    return index_count;
}

uint64_t Surface::get_indices_byte_size() const
{
    return index_count * sizeof(uint32_t);
}

void Surface::render_gui()
{

}

AABB Surface::get_aabb() const
{
    return this->aabb;
}

void Surface::set_aabb(AABB aabb)
{
    this->aabb = aabb;
}
