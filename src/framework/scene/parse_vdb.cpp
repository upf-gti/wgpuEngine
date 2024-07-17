#include "parse_vdb.h"

#include "glm/glm.hpp"

#include "framework/nodes/camera.h"
#include "framework/nodes/mesh_instance_3d.h"

#include "graphics/texture.h"
#include "graphics/shader.h"
#include "graphics/renderer_storage.h"

#include "engine/scene.h"

#include "spdlog/spdlog.h"

#include "shaders/volumetrics.wgsl.gen.h"

#include <fstream>

#include <openvdbReader.h>

void create_material_texture(int tex_index, Texture** texture)
{

}

bool parse_vdb(const char* vdb_path, std::vector<Node*>& entities)
{
    // loader
    std::string err;
    std::string warn;

    // Open file
    std::ifstream input_file(vdb_path, std::ios::binary | std::ios::ate); // ate changes the seek pointer to the end of the file

    // tellg returns pos_type which is a fpos object with a `.operator streamoff()`.
    const auto eof_position = static_cast<std::streamoff>(input_file.tellg());
    auto buffer = std::vector<uint8_t>(eof_position);

    // Change the seek pointer to the beginning
    input_file.seekg(0, std::ios::beg);
    // Copy all the bytes from from the the file into the vector named return
    input_file.read(reinterpret_cast<char*>(buffer.data()), eof_position);

    OpenVDBReader* vdbReader = new OpenVDBReader();
    //vdbReader->read(buffer);

    //

    MeshInstance3D* a = new MeshInstance3D();

    Surface* s = new Surface();
    s->create_box();
    s->set_name("VDB node");
    a->add_surface(s);

    Material m;
    m.color = { 1.0f, 0.0f, 0.0f, 1.0f };
    Texture* t = new Texture();

    uint32_t resolution = 100;
    unsigned int size = pow(resolution, 3);
    float* data = new float[size]; // GLubyte, uint8_t;
    for (unsigned int i = 0; i < size; i++) {
        data[i] = 1.0f;
    }

    //t->create(WGPUTextureDimension_3D, WGPUTextureFormat_R32Float, { resolution, resolution, resolution },
    //    static_cast<WGPUTextureUsage>(WGPUTextureUsage_StorageBinding | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst), 6, 1, data);
    t->load_from_data(s->get_name(), WGPUTextureDimension_3D, resolution, resolution, resolution, data, false, WGPUTextureFormat_R32Float);

    m.diffuse_texture = t;
    m.shader = RendererStorage::get_shader_from_source(shaders::volumetrics::source, shaders::volumetrics::path, m);



    a->set_surface_material_override(s, m);

    entities.push_back(a);


    return true;
}
