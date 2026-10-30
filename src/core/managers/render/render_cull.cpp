#include "render_cull.h"

#include "core/managers/debug/debug_manager.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/simulation/simulation_manager.h"
#include "core/managers/xr/xr_manager.h"

#include "core/managers/render/render_storage.h"
#include "graphics/mesh.h"
#include "graphics/uniform.h"

#include "glm/gtc/matrix_transform.hpp"

#include <vector>

Error RenderCull::initialize()
{
    return Error::OK;
}

Error RenderCull::finalize()
{
    delete render_method;
    render_method = nullptr;

    return Error::OK;
}

void RenderCull::set_render_method(RenderMethod* method)
{
    render_method = method;
}

RenderMethod* RenderCull::get_render_method()
{
    return render_method;
}

void RenderCull::set_render_size(int32_t width, int32_t height)
{
    render_method->set_render_size(width, height);
}

void RenderCull::set_frustum_camera_paused(bool value)
{
    frustum_camera_paused = value;
}

void RenderCull::render(const std::vector<sRenderableData>& renderables_list, WGPUTextureView screen_surface_texture_view, const sEnvironmentData& environment_data)
{
    bool xr_available = XR_MANAGER->is_xr_available();

    std::vector<std::vector<sRenderData>> render_lists(RENDER_LIST_COUNT);

    Camera3D* main_camera = nullptr;

    Camera3D eye_camera[EYE_COUNT];
    Camera3D eye_join_camera;

    if (xr_available) {
        XRContext* xr_context = XR_MANAGER->xr_context;
        assert(xr_context);

        // prepare eye cameras
        for (uint32_t eye_idx = 0; eye_idx < EYE_COUNT; eye_idx++) {
            eye_camera[eye_idx].set_eye(xr_context->per_view_data[eye_idx].position);
            eye_camera[eye_idx].set_view(xr_context->per_view_data[eye_idx].view_matrix, false);
            eye_camera[eye_idx].set_projection(xr_context->per_view_data[eye_idx].projection_matrix, false);
            eye_camera[eye_idx].set_view_projection(xr_context->per_view_data[eye_idx].view_projection_matrix);
        }

        eye_join_camera.set_eye((eye_camera[EYE_LEFT].get_eye() + eye_camera[EYE_RIGHT].get_eye()) * 0.5f);

        // Interpolate view
        {
            glm::mat4 left_view = xr_context->per_view_data[EYE_LEFT].view_matrix;
            glm::mat4 right_view = xr_context->per_view_data[EYE_RIGHT].view_matrix;
            glm::mat4 combined_view = left_view * 0.5f + right_view * 0.5f;
            eye_join_camera.set_view(combined_view, false);
        }

        // Set new FOV in projection
        {
            glm::mat4 left_proj = xr_context->per_view_data[EYE_LEFT].projection_matrix;
            float aspect = left_proj[1][1] / left_proj[0][0];
            if (!std::isnan(aspect)) {
                float eye_fov = 2.0f * atan(1.0f / left_proj[1][1]);
                float combined_eye_tan = tan(eye_fov / 2.0f) * 2.0f; // assuming same fov for both eyes
                float combined_fov = 2.0f * atan(combined_eye_tan);
                eye_join_camera.set_projection(glm::perspective(combined_fov, aspect, xr_context->z_near, xr_context->z_far));
            }
        }

        main_camera = &eye_join_camera;
    } else {
        main_camera = SIMULATION_MANAGER->get_main_camera();
    }

    if (main_camera) {
        prepare_cull_instancing(main_camera, renderables_list, render_lists);

        if (!xr_available) {
            render_method->render_scene(render_lists, main_camera, environment_data, screen_surface_texture_view, EYE_LEFT);
        } else {
            XRContext* xr_context = XR_MANAGER->xr_context;
            assert(xr_context);

            for (uint32_t i = 0; i < EYE_COUNT; i++) {
                xr_context->acquire_swapchain(i);

                render_method->render_scene(render_lists, &eye_camera[i], environment_data, xr_context->get_swapchain_view(i), static_cast<eEYE>(i));

                xr_context->release_swapchain(i);
            }
        }
    } else {
        LOG_ERROR("No main Camera in Scene");
    }
}

void RenderCull::prepare_cull_instancing(const Camera3D* camera, const std::vector<sRenderableData>& renderables_list, std::vector<std::vector<sRenderData>>& render_lists)
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
}
