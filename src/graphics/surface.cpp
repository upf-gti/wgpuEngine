#include "surface.h"

#include "framework/math/math_utils.h"
#include "framework/colors.h"

#include "graphics/renderer_storage.h"

#include "spdlog/spdlog.h"

WebGPUContext* Surface::webgpu_context = nullptr;
Surface* Surface::quad_mesh = nullptr;

Surface::~Surface()
{
    if (vertex_pos_buffer) {
        wgpuBufferDestroy(vertex_pos_buffer);
    }

    if (vertex_data_buffer) {
        wgpuBufferDestroy(vertex_data_buffer);
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

void Surface::update_vertex_buffer(const std::vector<glm::vec3>& vertices)
{
    if (vertices.size() != vertex_count && vertex_pos_buffer) {
        wgpuBufferDestroy(vertex_pos_buffer);
        vertex_pos_buffer = nullptr;

        vertex_count = vertices.size();
    }

    if (!vertex_pos_buffer) {
        vertex_pos_buffer = webgpu_context->create_buffer(get_vertices_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, vertices.data(), ("vertex_buffer_" + name).c_str());
    }
    else {
        webgpu_context->update_buffer(vertex_pos_buffer, 0, vertices.data(), get_vertices_byte_size());
    }
}

void Surface::update_surface_data(const sSurfaceData& vertices_data, bool store_data)
{
    if (store_data) {
        surface_data = vertices_data;
    }

    if (vertices_data.vertices.size() != vertex_count && vertex_pos_buffer) {
        wgpuBufferDestroy(vertex_pos_buffer);
        vertex_pos_buffer = nullptr;
    }

    if (!vertex_pos_buffer) {
        create_surface_data(vertices_data);
    }
    else {
        vertex_count = vertices_data.vertices.size();
        webgpu_context->update_buffer(vertex_pos_buffer, 0, vertices_data.vertices.data(), get_vertices_byte_size());

        const std::vector<sInterleavedData>& interleaved_data = create_interleaved_data(vertices_data);
        webgpu_context->update_buffer(vertex_data_buffer, 0, interleaved_data.data(), get_interleaved_data_byte_size());
    }
}

void Surface::create_surface_data(const sSurfaceData& vertices_data, bool store_data)
{
    if (store_data) {
        surface_data = vertices_data;
    }

    vertex_count = vertices_data.vertices.size();
    vertex_pos_buffer = webgpu_context->create_buffer(get_vertices_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, vertices_data.vertices.data(), ("vertex_buffer_" + name).c_str());

    const std::vector<sInterleavedData>& interleaved_data = create_interleaved_data(vertices_data);
    vertex_data_buffer = webgpu_context->create_buffer(get_interleaved_data_byte_size(), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex, interleaved_data.data(), ("vertex_data_buffer_" + name).c_str());

    if (!vertices_data.indices.empty()) {
        create_index_buffer(vertices_data.indices);
    }
}

void Surface::set_surface_data(const sSurfaceData& vertices_data)
{
    surface_data = vertices_data;
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

const sSurfaceData& Surface::get_surface_data() const
{
    return surface_data;
}

sSurfaceData& Surface::get_surface_data()
{
    return surface_data;
}

const WGPUBuffer& Surface::get_vertex_buffer() const
{
    return vertex_pos_buffer;
}

const WGPUBuffer& Surface::get_vertex_data_buffer() const
{
    return vertex_data_buffer;
}

const WGPUBuffer& Surface::get_index_buffer() const
{
    return index_buffer;
}

void Surface::create_axis(float s)
{
    sSurfaceData vertices;

    vertices.vertices.push_back({ glm::vec3(-1.0f,  0.0f,  0.0f) * s });
    vertices.vertices.push_back({ glm::vec3( 1.0f,  0.0f,  0.0f) * s });
    vertices.vertices.push_back({ glm::vec3( 0.0f, -1.0f,  0.0f) * s });
    vertices.vertices.push_back({ glm::vec3( 0.0f,  1.0f,  0.0f) * s });
    vertices.vertices.push_back({ glm::vec3( 0.0f,  0.0f, -1.0f) * s });
    vertices.vertices.push_back({ glm::vec3( 0.0f,  0.0f,  1.0f) * s });

    create_surface_data(vertices);
}

sSurfaceData Surface::generate_quad(float w, float h, const glm::vec3& position, const glm::vec3& normal, const glm::vec3& color, bool flip_y)
{
    struct {
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normal;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> color;
    } points;

    points.vertices.reserve(4);
    points.normal.reserve(4);
    points.uv.reserve(4);
    points.color.reserve(4);

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
            points.vertices.push_back(position - (orig + float(i1) * delta1 + float(i2) * delta2));
            points.normal.push_back(n);
            points.uv.push_back(glm::vec2(i1, flip_y ? (1.0f - i2) : i2));
            points.color.push_back(color);
        }
    }

    sSurfaceData vertices;
    vertices.vertices.resize(6);
    vertices.normals.resize(6);
    vertices.uvs.resize(6);
    vertices.colors.resize(6);

    counter = 0;

    auto add_vertex = [&](uint32_t idx) {
        vertices.vertices[counter] = points.vertices[idx];
        vertices.normals[counter] = points.normal[idx];
        vertices.uvs[counter] = points.uv[idx];
        vertices.colors[counter] = points.color[idx];
        counter++;
    };

    if (flip_y) {
        add_vertex(2);
        add_vertex(1);
        add_vertex(0);

        add_vertex(2);
        add_vertex(3);
        add_vertex(1);
    }
    else {
        add_vertex(0);
        add_vertex(1);
        add_vertex(2);

        add_vertex(1);
        add_vertex(3);
        add_vertex(2);
    }

    return vertices;
}

void Surface::create_quad(float w, float h, bool flip_y, bool centered, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_pos_buffer) {
        wgpuBufferDestroy(vertex_pos_buffer);
    }

    name = "quad";

    glm::vec3 origin(0.0f);

    if (!centered) {
        origin += glm::vec3(w * 0.5f, h * 0.5f, 0.0f);
    }

    sSurfaceData data = generate_quad(w, h, origin, normals::pZ, color, flip_y);

    spdlog::trace("Quad mesh created ({} vertices)", data.vertices.size());

    create_surface_data(data);
}

void Surface::create_subdivided_quad(float w, float h, bool flip_y, uint32_t subdivisions, bool centered, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_pos_buffer) {
        wgpuBufferDestroy(vertex_pos_buffer);
    }

    name = "subdivided_quad";

    sSurfaceData vertices;

    float step_x = w / static_cast<float>(subdivisions);
    float step_y = h / static_cast<float>(subdivisions);

    glm::vec3 origin = { -w * 0.5f, h * 0.5f, 0.0f };

    if (!centered) {
        origin += glm::vec3(w * 0.5f, h * 0.5f, 0.0f);
    }

    // Generate vertices with positions and UVs
    for (int i = 0; i < subdivisions; ++i) {
        for (int j = 0; j < subdivisions; ++j) {

            const glm::vec3& new_origin = origin + glm::vec3(j * step_x, -i * step_y, 0.0f) + glm::vec3(step_x * 0.5f, -step_y * 0.5f, 0.0f);
            sSurfaceData vtxs = generate_quad(step_x, step_y, new_origin, normals::pZ, color, flip_y);

            const glm::vec2& uv0 = { static_cast<float>(j) / subdivisions, static_cast<float>(i) / subdivisions };
            const glm::vec2& uv1 = { static_cast<float>(j + 1u) / subdivisions, static_cast<float>(i + 1u) / subdivisions };

            vtxs.uvs[0] = glm::vec2(uv1.x, 1.0f - uv0.y);
            vtxs.uvs[1] = glm::vec2(uv0.x, 1.0f - uv1.y);
            vtxs.uvs[2] = glm::vec2(uv0.x, 1.0f - uv0.y);

            vtxs.uvs[3] = glm::vec2(uv1.x, 1.0f - uv0.y);
            vtxs.uvs[4] = glm::vec2(uv1.x, 1.0f - uv1.y);
            vtxs.uvs[5] = glm::vec2(uv0.x, 1.0f - uv1.y);

            vertices.append(vtxs);
        }
    }

    spdlog::trace("Subvidided Quad mesh created ({} vertices)", vertices.vertices.size());

    create_surface_data(vertices);
}

void Surface::create_box(float w, float h, float d, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_pos_buffer) {
        wgpuBufferDestroy(vertex_pos_buffer);
    }

    name = "box";

    sSurfaceData vertices;

    float w2 = w * 2.0f;
    float h2 = h * 2.0f;
    float d2 = d * 2.0f;

    auto pos_x = generate_quad(h2, w2, d * normals::pX, -normals::pX, color, true);
    vertices.append(pos_x);
    auto neg_x = generate_quad(h2, w2, d * normals::nX, -normals::nX, color, true);
    vertices.append(neg_x);

    auto pos_y = generate_quad(w2, d2, h * normals::pY, -normals::pY, color, true);
    vertices.append(pos_y);
    auto neg_y = generate_quad(w2, d2, h * normals::nY, -normals::nY, color, true);
    vertices.append(neg_y);

    auto pos_z = generate_quad(w2, h2, d * normals::pZ, -normals::pZ, color, true);
    vertices.append(pos_z);
    auto neg_z = generate_quad(w2, h2, d * normals::nZ, -normals::nZ, color, true);
    vertices.append(neg_z);

    spdlog::trace("Box mesh created ({} vertices)", vertices.vertices.size());

    create_surface_data(vertices);
}

void Surface::create_rounded_box(float w, float h, float d, float c, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_pos_buffer) {
        wgpuBufferDestroy(vertex_pos_buffer);
    }

    name = "rounded_box";

    sSurfaceData vertices;

    w -= c;
    h -= c;

    float w2 = w * 2.0f;
    float h2 = h * 2.0f;
    float d2 = (d - c) * 2.0f;

    // Add side vertices adding the chamfer translation
    auto pos_x = generate_quad(h2, w2, d * normals::pX, -normals::pX, color, true);
    vertices.append(pos_x);
    auto neg_x = generate_quad(h2, w2, d * normals::nX, -normals::nX, color, true);
    vertices.append(neg_x);

    auto pos_y = generate_quad(w2, d2, (h + c) * normals::pY, -normals::pY, color, true);
    vertices.append(pos_y);
    auto neg_y = generate_quad(w2, d2, (h + c) * normals::nY, -normals::nY, color, true);
    vertices.append(neg_y);

    auto pos_z = generate_quad(w2, h2, d * normals::pZ, -normals::pZ, color, true);
    vertices.append(pos_z);
    auto neg_z = generate_quad(w2, h2, d * normals::nZ, -normals::nZ, color, true);
    vertices.append(neg_z);

    if (c > 0.0f) {

        float     pi = glm::pi<float>();
        float     half_pi = glm::pi<float>() * 0.5f;
        constexpr uint32_t  chamfer_seg = 8;

        d -= c;

        auto add_corner = [&](bool isXPositive, bool isYPositive, bool isZPositive)
        {
            sSurfaceData vtxs;
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
                    
                    vtxs.vertices[vtx_counter] = glm::vec3(x0 + offsetPosition.x, y0 + offsetPosition.y, z0 + offsetPosition.z);
                    vtxs.uvs[vtx_counter] = glm::vec2((float)seg / (float)chamfer_seg, (float)ring / (float)chamfer_seg);
                    vtxs.normals[vtx_counter] = glm::normalize(glm::vec3(x0, y0, z0));
                    vtxs.tangents[vtx_counter] = glm::vec4(0.0f);
                    vtxs.colors[vtx_counter] = color;
                    vtx_counter++;

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
            vertices.append(vtxs);
            size_t vertex_offset = vertices.size();
            vertices.resize(vertex_offset + indices.size());
            for (size_t i = 0; i < indices.size(); i++) {
                vertices.vertices[vertex_offset + i] = vtxs.vertices[indices[i]];
                vertices.uvs[vertex_offset + i] = vtxs.uvs[indices[i]];
                vertices.normals[vertex_offset + i] = vtxs.normals[indices[i]];
                vertices.tangents[vertex_offset + i] = vtxs.tangents[indices[i]];
                vertices.colors[vertex_offset + i] = vtxs.colors[indices[i]];
            }
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
            sSurfaceData vtxs;
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

                    vtxs.vertices[vtx_counter] = glm::vec3(x0 * vx0 + i * deltaHeight * vy0 + z0 * vz0 + offsetPosition);
                    vtxs.uvs[vtx_counter] = glm::vec2(j / (float)chamfer_seg, i / (float)numSegHeight);
                    vtxs.normals[vtx_counter] = glm::normalize(glm::vec3(x0 * vx0 + z0 * vz0));
                    vtxs.tangents[vtx_counter] = glm::vec4(0.0f);
                    vtxs.colors[vtx_counter] = color;
                    vtx_counter++;

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
            for (size_t i = 0; i < indices.size(); i++) {
                vertices.vertices[vertex_offset + i] = vtxs.vertices[indices[i]];
                vertices.uvs[vertex_offset + i] = vtxs.uvs[indices[i]];
                vertices.normals[vertex_offset + i] = vtxs.normals[indices[i]];
                vertices.tangents[vertex_offset + i] = vtxs.tangents[indices[i]];
                vertices.colors[vertex_offset + i] = vtxs.colors[indices[i]];
            }
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

    create_surface_data(vertices);
}

void Surface::create_sphere(float r, uint32_t segments, uint32_t rings, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_pos_buffer)
    {
        wgpuBufferDestroy(vertex_pos_buffer);
    }

    name = "sphere";

    sSurfaceData vertices;

    sSurfaceData vtxs;
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
            vtxs.vertices[vtx_counter] = glm::vec3(x0, y0, z0);
            vtxs.uvs[vtx_counter] = glm::vec2((float)seg / (float)segments, (float)ring / (float)rings);
            vtxs.normals[vtx_counter] = glm::normalize(glm::vec3(x0, y0, z0));
            vtxs.colors[vtx_counter] = glm::vec3(1.0f);

            vtx_counter++;

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
    for (uint32_t i = 0; i < indices.size(); i++) {
        vertices.vertices[i] = vtxs.vertices[indices[i]];
        vertices.uvs[i] = vtxs.uvs[indices[i]];
        vertices.normals[i] = vtxs.normals[indices[i]];
        vertices.colors[i] = vtxs.colors[indices[i]];
    }

    spdlog::trace("Sphere mesh created ({} vertices)", vertices.size());

    create_surface_data(vertices);
}

void Surface::create_cone(float r, float h, uint32_t segments, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_pos_buffer)
    {
        wgpuBufferDestroy(vertex_pos_buffer);
    }

    name = "cone";

    sSurfaceData vertices;

    vertices.resize(segments * 6 * 2);

    float pi2 = glm::pi<float>() * 2.f;
    float deltaAngle = pi2 / float(segments);
    float normal_y = r / h;
    uint32_t vtx_counter = 0;

    auto add_vertex = [&](const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv, const glm::vec3& color = glm::vec3(1.0f)) {
        vertices.vertices[vtx_counter] = p;
        vertices.normals[vtx_counter] = n;
        vertices.uvs[vtx_counter] = uv;
        vertices.colors[vtx_counter] = color;
        vtx_counter++;
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

    create_surface_data(vertices);
}

void Surface::create_cylinder(float r, float h, uint32_t segments, bool capped, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_pos_buffer)
    {
        wgpuBufferDestroy(vertex_pos_buffer);
    }

    name = "cylinder";

    sSurfaceData vertices;

    vertices.resize(segments * 6);

    float pi2 = glm::pi<float>() * 2.f;
    float deltaAngle = pi2 / float(segments);
    uint32_t vtx_counter = 0;

    auto add_vertex = [&](const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv, const glm::vec3& color = glm::vec3(1.0f)) {
        vertices.vertices[vtx_counter] = p;
        vertices.normals[vtx_counter] = n;
        vertices.uvs[vtx_counter] = uv;
        vertices.colors[vtx_counter] = color;
        vtx_counter++;
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

    create_surface_data(vertices);
}

void Surface::create_capsule(float r, float h, uint32_t segments, uint32_t rings, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_pos_buffer)
    {
        wgpuBufferDestroy(vertex_pos_buffer);
    }

    name = "capsule";

    sSurfaceData vertices;

    float pi = glm::pi<float>() * 2.f;
    float half_pi = glm::pi<float>() * 0.5f;
    float pi2 = glm::pi<float>() * 2.f;

    float delta_ring_angle = (half_pi / rings);
    float delta_seg_angle = (pi2 / segments);

    float sphere_ratio = r / (2 * r + h);
    float cylinder_ratio = h / (2 * r + h);

    uint32_t num_height_seg = 2;

    sSurfaceData vtxs;
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
            vtxs.vertices[vtx_counter] = glm::vec3(x0, 0.5f * h + y0, z0);
            vtxs.uvs[vtx_counter] = glm::vec2((float)seg / (float)segments, (float)ring / (float)rings * sphere_ratio);
            vtxs.normals[vtx_counter] = glm::normalize(glm::vec3(x0, y0, z0));
            vtxs.colors[vtx_counter] = color;
            vtx_counter++;

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

            vtxs.vertices[vtx_counter] = glm::vec3(x0, 0.5f * h - i * deltah, z0);
            vtxs.uvs[vtx_counter] = glm::vec2(j / (float)segments, (i / float(num_height_seg)) * cylinder_ratio + sphere_ratio);
            vtxs.normals[vtx_counter] = glm::normalize(glm::vec3(x0, 0, z0));
            vtxs.colors[vtx_counter] = color;
            vtx_counter++;

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
            vtxs.vertices[vtx_counter] = glm::vec3(x0, -0.5f * h + y0, z0);
            vtxs.uvs[vtx_counter] = glm::vec2((float)seg / (float)segments, (float)ring / (float)rings * sphere_ratio + cylinder_ratio + sphere_ratio);
            vtxs.normals[vtx_counter] = glm::normalize(glm::vec3(x0, y0, z0));
            vtxs.colors[vtx_counter] = color;
            vtx_counter++;

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
    for (uint32_t i = 0; i < indices.size(); i++) {
        vertices.vertices[i] = vtxs.vertices[indices[i]];
        vertices.uvs[i] = vtxs.uvs[indices[i]];
        vertices.normals[i] = vtxs.normals[indices[i]];
        vertices.colors[i] = vtxs.colors[indices[i]];
    }

    spdlog::trace("Capsule mesh created ({} vertices)", vertices.size());

    create_surface_data(vertices);
}

void Surface::create_torus(float r, float ir, uint32_t segments_section, uint32_t segments_circle, const glm::vec3& color)
{
    // Mesh has vertex data...
    if (vertex_pos_buffer)
    {
        wgpuBufferDestroy(vertex_pos_buffer);
    }

    name = "torus";

    sSurfaceData vertices;

    sSurfaceData vtxs;
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

            vtxs.vertices[vtx_counter] = v;
            vtxs.uvs[vtx_counter] = glm::vec2(i / (float)segments_circle, j / (float)segments_section);
            vtxs.normals[vtx_counter] = glm::normalize(v - c);
            vtxs.colors[vtx_counter] = color;
            vtx_counter++;

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
    for (uint32_t i = 0; i < indices.size(); i++) {
        vertices.vertices[i] = vtxs.vertices[indices[i]];
        vertices.uvs[i] = vtxs.uvs[indices[i]];
        vertices.normals[i] = vtxs.normals[indices[i]];
        vertices.colors[i] = vtxs.colors[indices[i]];
    }

    spdlog::trace("Torus mesh created ({} vertices)", vertices.size());

    create_surface_data(vertices);
}

void Surface::create_circle(float radius, uint32_t segments)
{
    name = "circle";

    sSurfaceData vertices;

    float increment = 2.0f * PI / segments;

    for (float currAngle = 0.0f; currAngle <= 2.0f * PI + increment; currAngle += increment)
    {
        vertices.vertices.push_back( glm::vec3(radius * cos(currAngle), radius * sin(currAngle), 0) );
        vertices.colors.push_back( glm::vec3(1.0f) );
    }

    spdlog::trace("Circle mesh created ({} vertices)", vertices.size());

    create_surface_data(vertices);
}

void Surface::create_arrow()
{
    name = "arrow";


    sSurfaceData vertices;
    vertices.resize(8);

    uint32_t vtx_counter = 0;
    auto add_vertex = [&](const glm::vec3& p, const glm::vec3& color = glm::vec3(1.0f)) {
        vertices.vertices[vtx_counter] = p;
        vertices.colors[vtx_counter] = color;
        vtx_counter++;
    };

    add_vertex(glm::vec3(1.0f, 0.0f, 0.0f));
    add_vertex(glm::vec3(0.6f, 0.25f, 0.0f));
    add_vertex(glm::vec3(0.6f, 0.1f, 0.0f));
    add_vertex(glm::vec3(0.0f, 0.1f, 0.0f));
    add_vertex(glm::vec3(0.0f, -0.1f, 0.0f));
    add_vertex(glm::vec3(0.6f, -0.1f, 0.0f));
    add_vertex(glm::vec3(0.6f, -0.25f, 0.0f));
    add_vertex(glm::vec3(1.0f, 0.0f, 0.0f));

    spdlog::trace("Circle mesh created ({} vertices)", vertices.size());

    create_surface_data(vertices);
}

void Surface::create_skybox()
{
    name = "skybox";

    sSurfaceData vertices;
    vertices.vertices.resize(36);
    vertices.colors.resize(36);

    uint32_t vtx_counter = 0;
    auto add_vertex = [&](const glm::vec3& p, const glm::vec3& color = glm::vec3(1.0f)) {
        vertices.vertices[vtx_counter] = p;
        vertices.colors[vtx_counter] = color;
        vtx_counter++;
    };

    add_vertex({ -1.0f, 1.0f, -1.0f } );
    add_vertex({ -1.0f, -1.0f, -1.0f } );
    add_vertex({  1.0f, -1.0f, -1.0f } );
    add_vertex({  1.0f, -1.0f, -1.0f } );
    add_vertex({  1.0f,  1.0f, -1.0f } );
    add_vertex({ -1.0f,  1.0f, -1.0f } );

    add_vertex({ -1.0f, -1.0f,  1.0f } );
    add_vertex({ -1.0f, -1.0f, -1.0f } );
    add_vertex({ -1.0f,  1.0f, -1.0f } );
    add_vertex({ -1.0f,  1.0f, -1.0f } );
    add_vertex( { -1.0f,  1.0f,  1.0f } );
    add_vertex( { -1.0f, -1.0f,  1.0f } );

    add_vertex( { 1.0f, -1.0f, -1.0f } );
    add_vertex( { 1.0f, -1.0f,  1.0f } );
    add_vertex( { 1.0f,  1.0f,  1.0f } );
    add_vertex( { 1.0f,  1.0f,  1.0f } );
    add_vertex( { 1.0f,  1.0f, -1.0f } );
    add_vertex( { 1.0f, -1.0f, -1.0f } );

    add_vertex( { -1.0f, -1.0f,  1.0f } );
    add_vertex( { -1.0f,  1.0f,  1.0f } );
    add_vertex( {  1.0f,  1.0f,  1.0f } );
    add_vertex( {  1.0f,  1.0f,  1.0f } );
    add_vertex( {  1.0f, -1.0f,  1.0f } );
    add_vertex( { -1.0f, -1.0f,  1.0f } );

    add_vertex( { -1.0f,  1.0f, -1.0f } );
    add_vertex( { 1.0f,  1.0f, -1.0f } );
    add_vertex( { 1.0f,  1.0f,  1.0f } );
    add_vertex( { 1.0f,  1.0f,  1.0f } );
    add_vertex( { -1.0f,  1.0f,  1.0f } );
    add_vertex( { -1.0f,  1.0f, -1.0f } );

    add_vertex( { -1.0f, -1.0f, -1.0f } );
    add_vertex( { -1.0f, -1.0f,  1.0f } );
    add_vertex( {  1.0f, -1.0f, -1.0f } );
    add_vertex( {  1.0f, -1.0f, -1.0f } );
    add_vertex( { -1.0f, -1.0f,  1.0f } );
    add_vertex( { 1.0f, -1.0f,  1.0f } );

    create_surface_data(vertices);
}

std::vector<sInterleavedData> Surface::create_interleaved_data(const sSurfaceData& vertices_data)
{
    std::vector<sInterleavedData> interleaved_data;
    interleaved_data.reserve(vertices_data.size());

    for (uint32_t idx = 0; idx < vertices_data.size(); idx++) {

        interleaved_data.push_back({
            vertices_data.uvs.empty() ? glm::vec2(0.0f) : vertices_data.uvs[idx],
            vertices_data.normals.empty() ? glm::vec3(0.0f) : vertices_data.normals[idx],
            vertices_data.tangents.empty() ? glm::vec4(0.0f) : vertices_data.tangents[idx],
            vertices_data.colors.empty() ? glm::vec3(1.0f) : vertices_data.colors[idx],
            vertices_data.weights.empty() ? glm::vec4(0.0f) : vertices_data.weights[idx],
            vertices_data.joints.empty() ? glm::ivec4(0) : vertices_data.joints[idx],
        });
    }

    return interleaved_data;
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
    return vertex_count * sizeof(glm::vec3);
}

uint64_t Surface::get_interleaved_data_byte_size() const
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

int Surface::mikkt_get_num_faces(const SMikkTSpaceContext* pContext)
{
    sSurfaceData& triangle_data = *reinterpret_cast<sSurfaceData*>(pContext->m_pUserData);

    if (!triangle_data.indices.empty()) {
        return triangle_data.indices.size() / 3;
    }
    else {
        return triangle_data.vertices.size() / 3;
    }
}

int Surface::mikkt_get_num_vertices_of_face(const SMikkTSpaceContext* pContext, const int iFace)
{
    return 3; //always 3
}

void Surface::mikkt_get_position(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
{
    sSurfaceData& triangle_data = *reinterpret_cast<sSurfaceData*>(pContext->m_pUserData);
    if (!triangle_data.indices.empty()) {
        uint32_t index = triangle_data.indices[iFace * 3 + iVert];
        if (index < triangle_data.vertices.size()) {
            memcpy(fvPosOut, &triangle_data.vertices[index], sizeof(glm::vec3));
        }
    }
    else {
        memcpy(fvPosOut, &triangle_data.vertices[iFace * 3 + iVert], sizeof(glm::vec3));
    }
}

void Surface::mikkt_get_normal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
{
    sSurfaceData& triangle_data = *reinterpret_cast<sSurfaceData*>(pContext->m_pUserData);
    if (!triangle_data.indices.empty()) {
        uint32_t index = triangle_data.indices[iFace * 3 + iVert];
        if (index < triangle_data.vertices.size()) {
            memcpy(fvNormOut, &triangle_data.normals[index], sizeof(glm::vec3));
        }
    }
    else {
        memcpy(fvNormOut, &triangle_data.normals[iFace * 3 + iVert], sizeof(glm::vec3));
    }
}

void Surface::mikkt_get_tex_coord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
{
    sSurfaceData& triangle_data = *reinterpret_cast<sSurfaceData*>(pContext->m_pUserData);
    if (!triangle_data.indices.empty()) {
        uint32_t index = triangle_data.indices[iFace * 3 + iVert];
        if (index < triangle_data.vertices.size()) {
            memcpy(fvTexcOut, &triangle_data.uvs[index], sizeof(glm::vec2));
        }
    }
    else {
        memcpy(fvTexcOut, &triangle_data.uvs[iFace * 3 + iVert], sizeof(glm::vec2));
    }
}

void Surface::mikkt_set_tspace_basic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
{
    sSurfaceData& triangle_data = *reinterpret_cast<sSurfaceData*>(pContext->m_pUserData);
    glm::vec4* tangent = nullptr;
    if (!triangle_data.indices.empty()) {
        uint32_t index = triangle_data.indices[iFace * 3 + iVert];
        if (index < triangle_data.vertices.size()) {
            tangent = &triangle_data.tangents[index];
        }
    }
    else {
        tangent = &triangle_data.tangents[iFace * 3 + iVert];
    }

    if (tangent != nullptr) {
        *tangent = glm::vec4(fvTangent[0], fvTangent[1], fvTangent[2], -fSign);
    }
}

bool Surface::generate_tangents(sSurfaceData* vertices_data)
{
    vertices_data->tangents.resize(vertices_data->vertices.size());

    SMikkTSpaceInterface mkif;
    mkif.m_getNormal = mikkt_get_normal;
    mkif.m_getNumFaces = mikkt_get_num_faces;
    mkif.m_getNumVerticesOfFace = mikkt_get_num_vertices_of_face;
    mkif.m_getPosition = mikkt_get_position;
    mkif.m_getTexCoord = mikkt_get_tex_coord;
    mkif.m_setTSpace = nullptr;
    mkif.m_setTSpaceBasic = mikkt_set_tspace_basic;

    SMikkTSpaceContext msc;
    msc.m_pInterface = &mkif;
    msc.m_pUserData = vertices_data;

    if (!genTangSpaceDefault(&msc)) {
        spdlog::error("Could not generate tangents for surface: {}", name);
        vertices_data->tangents.clear();
        return false;
    }

    return true;
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

uint32_t sSurfaceData::size() const
{
    return vertices.size();
}

void sSurfaceData::resize(uint32_t size)
{
    vertices.resize(size);
    indices.resize(size);
    uvs.resize(size);
    normals.resize(size);
    tangents.resize(size);
    colors.resize(size);
    weights.resize(size);
    joints.resize(size);
}

void sSurfaceData::append(const sSurfaceData& surface_data)
{
    vertices.insert(vertices.end(), surface_data.vertices.begin(), surface_data.vertices.end());
    indices.insert(indices.end(), surface_data.indices.begin(), surface_data.indices.end());
    uvs.insert(uvs.end(), surface_data.uvs.begin(), surface_data.uvs.end());
    normals.insert(normals.end(), surface_data.normals.begin(), surface_data.normals.end());
    tangents.insert(tangents.end(), surface_data.tangents.begin(), surface_data.tangents.end());
    colors.insert(colors.end(), surface_data.colors.begin(), surface_data.colors.end());
    weights.insert(weights.end(), surface_data.weights.begin(), surface_data.weights.end());
    joints.insert(joints.end(), surface_data.joints.begin(), surface_data.joints.end());
}

void sSurfaceData::clear()
{
    vertices.clear();
    indices.clear();
    uvs.clear();
    normals.clear();
    tangents.clear();
    colors.clear();
    weights.clear();
    joints.clear();
}
