#include "render_cull.h"

#include "core/managers/render/render_manager.h"
#include "core/managers/render/render_methods/render_method.h"
#include "core/managers/simulation/simulation_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "core/managers/render/render_storage.h"
#include "graphics/mesh.h"
#include "graphics/uniform.h"

#include "imgui.h"

#include <vector>

Error RenderCull::initialize()
{
    return Error::OK;
}

Error RenderCull::finalize()
{
    return Error::OK;
}

void RenderCull::set_render_method(RenderMethod* method)
{
    render_method = method;
}

void RenderCull::set_frustum_camera_paused(bool value)
{
    frustum_camera_paused = value;
}

void RenderCull::render(const std::vector<sRenderableData>& renderables_list)
{
    bool xr_available = XRManager::get_singleton()->is_xr_available();

    WGPUTextureView screen_surface_texture_view;
    WGPUSurfaceTexture screen_surface_texture;

    std::vector<std::vector<sRenderData>> render_lists(RENDER_LIST_COUNT);

    wgpuSurfaceGetCurrentTexture(RenderAPI::get_singleton()->get_main_surface(), &screen_surface_texture);
    if (screen_surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
        // Probably minimized window
        ImGui::EndFrame();
        return;
    }

    screen_surface_texture_view = RenderAPI::get_singleton()->texture_view_create(screen_surface_texture.texture, WGPUTextureViewDimension_2D, RenderManager::get_singleton()->get_surface_format());

    if (!xr_available) {
        prepare_cull_instancing(SimulationManager::get_singleton()->get_main_camera(), renderables_list, render_lists);

        //glm::vec3 eye = camera_3d->get_eye();
        //glm::vec3 center = camera_3d->get_center();

        render_method->render_scene(render_lists);
    }
}

void RenderCull::prepare_cull_instancing(const Camera* camera, const std::vector<sRenderableData>& renderables_list, std::vector<std::vector<sRenderData>>& render_lists)
{
    if (!frustum_camera_paused) {
        frustum_cull.set_view_projection(camera->get_view_projection());
    }

    //render_entity_list.push_back({ skybox_mesh, glm::translate(glm::mat4(1.0f), get_camera_eye()) });

    // Get all surfaces from entity meshes
    for (auto render_list_data : renderables_list) {
        Mesh* mesh = render_list_data.mesh;
        glm::mat4x4 global_matrix = render_list_data.global_matrix;

        const std::vector<Surface*>& surfaces = mesh->get_surfaces();

        for (Surface* surface : surfaces) {
            Material* material_override = mesh->get_surface_material_override(surface);
            Material* material = material_override ? material_override : surface->get_material();

            //if (is_shadow_pass) {
            //    material = shadow_material;
            //}

            if (!material || !material->get_shader()) {
                continue;
            }

            bool material_is_2d = material->get_is_2D();

            //if (is_shadow_pass && (!mesh->get_receive_shadows() || material_is_2d || material->get_transparency_type() == ALPHA_BLEND)) {
            //    continue;
            //}

            if (!material_is_2d && mesh->get_frustum_culling_enabled()) {
                const AABB& surface_aabb = surface->get_aabb();

                AABB aabb_transformed = surface_aabb.transform(global_matrix);

                if (!frustum_cull.is_box_visible(aabb_transformed.center - aabb_transformed.half_size, aabb_transformed.center + aabb_transformed.half_size)) {
                    continue;
                }
            }

            RenderStorage::get_singleton()->register_material_bind_group(mesh, material);
            RenderStorage::get_singleton()->register_render_pipeline(material);

            eRenderListType list = RENDER_LIST_OPAQUE;

            if (material_is_2d) {
                list = material->get_transparency_type() == ALPHA_BLEND ? RENDER_LIST_2D_TRANSPARENT : RENDER_LIST_2D;
            } else if (material->get_transparency_type() == ALPHA_BLEND) {
                list = RENDER_LIST_TRANSPARENT;
            }

            render_lists[list].push_back({ surface, 1, global_matrix, mesh, material });
        }
    }

    for (int i = 0; i < RENDER_LIST_COUNT; ++i) {
        instances_data.instances_data[i].clear();
        instances_data.instances_data[i].resize(render_lists[i].size());

        /*if (i != RENDER_LIST_TRANSPARENT)*/ {
            // Sort opaques render_list
            std::sort(render_lists[i].begin(), render_lists[i].end(), [](auto& lhs, auto& rhs) {
                Material* lhs_mat = lhs.material;
                Material* rhs_mat = rhs.material;

                bool equal_priority = lhs_mat->get_priority() == rhs_mat->get_priority();
                bool equal_surface = lhs.surface == rhs.surface;

                if (lhs_mat->get_priority() > rhs_mat->get_priority()) {
                    return true;
                }
                if (equal_priority && lhs.surface > rhs.surface) {
                    return true;
                }
                if (equal_priority && equal_surface && lhs_mat > rhs_mat) {
                    return true;
                }

                return false;
            });
        }
        //else {
        //    // Sort transparent render_list by distance to camera
        //    std::sort(render_list[i].begin(), render_list[i].end(), [&](auto& lhs, auto& rhs) {
        //        glm::vec3 lhs_pos = glm::vec3(lhs.global_matrix[3]);
        //        glm::vec3 rhs_pos = glm::vec3(rhs.global_matrix[3]);

        //        float lhs_dist = glm::distance2(lhs_pos, camera_position);
        //        float rhs_dist = glm::distance2(rhs_pos, camera_position);

        //        if (lhs_dist > rhs_dist) return true;

        //        return false;
        //    });
        //}

        // Check instances
        {
            const Surface* prev_surface = nullptr;
            Material* prev_material = nullptr;

            uint32_t repeats = 0;
            for (uint32_t j = 0; j < render_lists[i].size(); ++j) {
                const sRenderData& render_data = render_lists[i][j];

                Material* material = render_data.material;

                // Repeated MeshInstance3D, must be instanced
                if (prev_surface == render_data.surface && prev_material == material && !material->get_is_2D()) {
                    repeats++;
                } else {
                    if (repeats > 0) {
                        for (uint32_t k = 1; k <= repeats; k++) {
                            render_lists[i][j - k].repeat = k;
                        }
                    }
                    repeats = 1;
                }

                prev_surface = render_data.surface;
                prev_material = material;

                // Fill instance_data
                instances_data.instances_data[i][j] = { render_data.global_matrix };
            }

            if (repeats > 0) {
                for (uint32_t k = 1; k <= repeats; k++) {
                    render_lists[i][render_lists[i].size() - k].repeat = k;
                }
            }
        }

        // Fill instance buffers
        uint32_t instances = static_cast<uint32_t>(instances_data.instances_data[i].size());

        if (instances > (instances_data.instances_data_uniforms[i].buffer_size / sizeof(sUniformData))) {
            //std::vector<sUniformData> default_data = { instances, { glm::mat4x4(1.0f), glm::vec4(1.0f) } };

            if (std::holds_alternative<WGPUBuffer>(instances_data.instances_data_uniforms[i].data)) {
                wgpuBufferDestroy(std::get<WGPUBuffer>(instances_data.instances_data_uniforms[i].data));
            }

            instances_data.instances_data_uniforms[i].data = RenderAPI::get_singleton()->buffer_create(sizeof(sUniformData) * instances, WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage, instances_data.instances_data[i].data(), "instance_mesh_buffer");
            instances_data.instances_data_uniforms[i].binding = 0;
            instances_data.instances_data_uniforms[i].buffer_size = sizeof(sUniformData) * instances;

            // Recreate bind groups
            std::vector<Uniform*> uniforms = { &instances_data.instances_data_uniforms[i] };
            Shader* prev_shader = nullptr;
            for (uint32_t j = 0; j < render_lists[i].size();) {
                const sRenderData& render_data = render_lists[i][j];

                if (instances_data.instances_bind_groups[i]) {
                    wgpuBindGroupRelease(instances_data.instances_bind_groups[i]);
                }

                instances_data.instances_bind_groups[i] = RenderAPI::get_singleton()->bind_group_create(uniforms, render_data.material->get_shader(), 0);

                j += render_data.repeat;
            }

        } else if (instances > 0) {
            RenderAPI::get_singleton()->buffer_update(std::get<WGPUBuffer>(instances_data.instances_data_uniforms[i].data), 0, instances_data.instances_data[i].data(), sizeof(sUniformData) * instances);
        }
    }
}
