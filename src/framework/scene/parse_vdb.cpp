#include "parse_vdb.h"

#include "glm/glm.hpp"

#include "framework/nodes/camera.h"
#include "framework/nodes/mesh_instance_3d.h"

#include "graphics/texture.h"
#include "graphics/shader.h"
#include "graphics/renderer_storage.h"

#include "engine/scene.h"

#include "spdlog/spdlog.h"

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
    vdbReader->read(buffer);


    return true;
}
