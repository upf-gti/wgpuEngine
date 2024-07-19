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

Material create_material_volume(OpenVDBReader* vdbReader)
{
    Material m;
    m.transparency_type = ALPHA_BLEND;
    m.shader = RendererStorage::get_shader("data/shaders/volumetrics.wgsl", m);

    uint32_t resolution = 100;
    float radius = 3.0;

    // now, read the grid from the vdbReader and store the data in a 3D texture
    int convertedGrids = 0;
    int convertedVoxels = 0;

    int totalGrids = vdbReader->gridsSize;
    int totalVoxels = totalGrids * pow(resolution, 3);

    float resolutionInv = 1.0f / resolution;
    int resolutionPow2 = pow(resolution, 2);
    int resolutionPow3 = pow(resolution, 3);

    // read all grids data and convert to texture
    for (unsigned int i = 0; i < totalGrids; i++) {
        Grid& grid = vdbReader->grids[i];
        float* data = new float[resolutionPow3];
        memset(data, 0, sizeof(float) * resolutionPow3);

        // Bbox
        Bbox bbox = Bbox();
        bbox = grid.getPreciseWorldBbox();
        glm::vec3 target = bbox.getCenter();
        glm::vec3 size = bbox.getSize();
        glm::vec3 step = size * resolutionInv;

        grid.transform->applyInverseTransformMap(step);
        target = target - (size * 0.5f);
        grid.transform->applyInverseTransformMap(target);
        target = target + (step * 0.5f);

        int x = 0;
        int y = 0;
        int z = 0;

        for (unsigned int j = 0; j < resolutionPow3; j++) {
            int baseX = x;
            int baseY = y;
            int baseZ = z;
            int baseIndex = baseX + baseY * resolution + baseZ * resolutionPow2;

            if (target.x >= 40 && target.y >= 40.33 && target.z >= 10.36) {
                int a = 0;
            }

            float value = grid.getValue(target);

            int cellBleed = radius;

            if (cellBleed) {
                for (int sx = -cellBleed; sx < cellBleed; sx++) {
                    for (int sy = -cellBleed; sy < cellBleed; sy++) {
                        for (int sz = -cellBleed; sz < cellBleed; sz++) {
                            if (x + sx < 0.0 || x + sx >= resolution ||
                                y + sy < 0.0 || y + sy >= resolution ||
                                z + sz < 0.0 || z + sz >= resolution) {
                                continue;
                            }

                            int targetIndex = baseIndex + sx + sy * resolution + sz * resolutionPow2;

                            float offset = std::max(0.0, std::min(1.0, 1.0 - std::hypot(sx, sy, sz) / (radius / 2.0)));
                            float dataValue = offset * value * 255.f;

                            data[targetIndex] += dataValue;
                            data[targetIndex] = std::min((float)data[targetIndex], 255.f);
                        }
                    }
                }
            }
            else {
                float dataValue = value * 255.f;

                data[baseIndex] += dataValue;
                data[baseIndex] = std::min((float)data[baseIndex], 255.f);
            }

            convertedVoxels++;

            if (z >= resolution) {
                break;
            }

            x++;
            target.x += step.x;

            if (x >= resolution) {
                x = 0;
                target.x -= step.x * resolution;

                y++;
                target.y += step.y;
            }

            if (y >= resolution) {
                y = 0;
                target.y -= step.y * resolution;

                z++;
                target.z += step.z;
            }

            // yield
        }

        // once the data array is filled, upload to the gpu as a 3d texture
        Texture* t = new Texture();
        t->load_from_data("VDB volume", WGPUTextureDimension_3D, resolution, resolution, resolution, data, false, WGPUTextureFormat_R32Float);

        if (grid.uniqueName == "density" || grid.gridName == "density") {
            m.diffuse_texture = t;
        }
        else if (grid.uniqueName == "flames" || grid.gridName == "flames") {
            // to do
        }
    }

    return m;
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

    // create a texture
    Material m = create_material_volume(vdbReader);

    MeshInstance3D* a = new MeshInstance3D();
    a->set_name("VDB node");

    Surface* s = new Surface();
    s->create_box();
    s->set_name("VDB surface");
    a->add_surface(s);

    a->set_surface_material_override(s, m);

    entities.push_back(a);

    return true;
}
