#include "debug_manager.h"

#include "core/managers/display/display_manager.h"
#include "core/managers/render/render_api.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/simulation/simulation_manager.h"

#include "core/managers/render/render_storage.h"
#include "graphics/primitives/box_mesh.h"
#include "graphics/primitives/capsule_mesh.h"
#include "graphics/primitives/cone_mesh.h"
#include "graphics/primitives/cylinder_mesh.h"
#include "graphics/primitives/sphere_mesh.h"
#include "graphics/primitives/torus_mesh.h"

#include "scene/3d/directional_light_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/omni_light_3d.h"
#include "scene/3d/spot_light_3d.h"
#include "scene/main/node.h"
#include "scene/main/scene.h"

#include "shaders/mesh_forward.wgsl.gen.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_wgpu.h"
#include "framework/utils/ImGuizmo.h"
#include "framework/utils/tinyfiledialogs.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "renderdoc_app.h"

#if defined(_WIN32)
#include "windows.h"
#elif defined(__linux__)
#include <dlfcn.h>
#endif

#include "GLFW/glfw3.h"

Error DebugManager::initialize()
{
    singleton_instance = this;

    initialize_renderdoc();
    initialize_imgui();

    return Error::OK;
}

Error DebugManager::finalize()
{
    singleton_instance = nullptr;

    delete rdoc_api;
    rdoc_api = nullptr;

    return Error::OK;
}

Error DebugManager::initialize_renderdoc()
{
    bool loaded = false;

#if defined(_WIN32)
    // At init, on windows
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI =
                (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        loaded = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdoc_api) == 1;
    }
#elif defined(__linux__)
    // At init, on linux/android.
    // For android replace librenderdoc.so with libVkLayer_GLES_RenderDoc.so
    if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)) {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
        loaded = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&rdoc_api) == 1;
    }
#endif

    if (!loaded) {
        return Error::FAILED;
    }

    return Error::OK;
}

Error DebugManager::renderdoc_start_capture()
{
    if (!rdoc_api) {
        return Error::FAILED;
    }

    rdoc_api->StartFrameCapture(nullptr, nullptr);

    return Error::OK;
}

Error DebugManager::renderdoc_end_capture()
{
    if (!rdoc_api) {
        return Error::FAILED;
    }

    rdoc_api->EndFrameCapture(nullptr, nullptr);

    return Error::OK;
}

Error DebugManager::initialize_imgui()
{
    // Init imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::StyleColorsDark(&style);
    // make it look a bit nicer with rounded edges
    style.WindowRounding = 2.0f;
    style.FrameRounding = 3.0f;
    style.FramePadding = ImVec2(6.0f, 3.0f);
    //style.ChildRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 3.0f;
    style.PopupRounding = 2.0f;

    ImGui::StyleColorsDark();

    float dpi = DisplayManager::get_singleton()->window_get_dpi();
    style.ScaleAllSizes(dpi);
    //style.FontScaleDpi = main_scale;

    ImFontConfig fontCfg = {};
    strcpy(fontCfg.Name, "ProggyForever.ttf");
    float fontSize = 12.0f * dpi;
    fontCfg.RasterizerDensity = dpi;
    float fontSizeInt = max(1.0f, roundf(fontSize));
    fontCfg.SizePixels = fontSizeInt;

    io.Fonts->AddFontDefaultVector(&fontCfg);

    ImGui_ImplGlfw_InitForOther(DisplayManager::get_singleton()->get_window(), true);

    ImGui_ImplWGPU_InitInfo init_info = {};
    init_info.Device = RenderAPI::get_singleton()->get_device();
    init_info.RenderTargetFormat = WGPUTextureFormat_BGRA8Unorm;
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    init_info.NumFramesInFlight = 3;

    ImGui_ImplWGPU_Init(&init_info);

    // Disable file-system access in web builds (don't load imgui.ini)
#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif

    return Error::OK;
}

void DebugManager::start_render_overlay()
{
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    int32_t render_width = RenderManager::get_singleton()->get_render_width();
    int32_t render_height = RenderManager::get_singleton()->get_render_height();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplayFramebufferScale = ImVec2(render_width / io.DisplaySize.x, render_height / io.DisplaySize.y);
    ImGui::NewFrame();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();
}

void DebugManager::render_default_gui()
{
    bool active = true;

    float main_menu_height = 0.0f;

    Scene* main_scene = SimulationManager::get_singleton()->get_main_scene();

    if (ImGui::BeginMainMenuBar()) {
        main_menu_height = ImGui::GetFrameHeight();
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open scene (.gltf, .glb, .obj, .vdb, .ply)")) {
                std::vector<const char*> filter_patterns = { "*.gltf", "*.glb", "*.obj", "*.vdb", "*.ply" };
                char const* open_file_name = tinyfd_openFileDialog(
                        "Scene loader",
                        "",
                        static_cast<int>(filter_patterns.size()),
                        filter_patterns.data(),
                        "Scene formats",
                        0);

                if (open_file_name) {
                    std::vector<Node*> entities;
                    //parse_scene(open_file_name, entities);
                    main_scene->add_nodes(entities);
                }
            }

            if (ImGui::MenuItem("Exit")) {
                exit(0);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Add")) {
            if (ImGui::BeginMenu("Mesh")) {
                auto create_mesh_instance = [&](Mesh* mesh) {
                    auto boxMaterial = new Material();
                    boxMaterial->set_shader(RenderStorage::get_singleton()->get_shader_from_source(shaders::mesh_forward::source, shaders::mesh_forward::path, shaders::mesh_forward::libraries, boxMaterial));
                    auto box = new MeshInstance3D();
                    box->set_name(mesh->get_mesh_type());
                    box->set_mesh(mesh);
                    box->set_surface_material_override(box->get_surface(0), boxMaterial);
                    main_scene->add_node(box);
                };

                if (ImGui::MenuItem("Box")) {
                    create_mesh_instance(new BoxMesh());
                }

                if (ImGui::MenuItem("Capsule")) {
                    create_mesh_instance(new CapsuleMesh());
                }

                if (ImGui::MenuItem("Cone")) {
                    create_mesh_instance(new ConeMesh());
                }

                if (ImGui::MenuItem("Cylinder")) {
                    create_mesh_instance(new CylinderMesh());
                }

                if (ImGui::MenuItem("Sphere")) {
                    create_mesh_instance(new SphereMesh());
                }

                if (ImGui::MenuItem("Torus")) {
                    create_mesh_instance(new TorusMesh());
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Light")) {
                auto create_light_instance = [&](Light3D* light) {
                    light->create_debug_meshes();
                    main_scene->add_node(light);
                };

                if (ImGui::MenuItem("DirectionalLight")) {
                    create_light_instance(new DirectionalLight3D());
                }

                if (ImGui::MenuItem("SpotLight")) {
                    create_light_instance(new SpotLight3D());
                }

                if (ImGui::MenuItem("OmniLight")) {
                    create_light_instance(new OmniLight3D());
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    float height = ImGui::GetFrameHeight();
    if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Down, height, window_flags)) {
        if (ImGui::BeginMenuBar()) {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("Performance %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

    float scene_tree_height = viewport->Size.y - height * 2.0f;

    // Set position to top-left of the viewport
    ImGui::SetNextWindowPos(viewport->WorkPos);

    float dpi = DisplayManager::get_singleton()->window_get_dpi();

    float panelWidth = 350.0f * dpi;
    ImGui::SetNextWindowSize({ panelWidth, scene_tree_height }, ImGuiCond_Once);

    ImGui::Begin("Debug panel", &active, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    bool scene_tab_open = false;
    if (ImGui::BeginTabBar("TabBar", tab_bar_flags)) {
        scene_tab_open = ImGui::BeginTabItem("Scene");
        if (scene_tab_open) {
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
            ImGui::BeginChild("SceneTree", ImVec2(0, 260), ImGuiChildFlags_Borders, ImGuiWindowFlags_None);

            std::vector<Node*>& nodes = main_scene->get_nodes();
            std::vector<Node*>::iterator it = nodes.begin();
            while (it != nodes.end()) {
                if (render_scene_tree_recursive(*it)) {
                    if (*it == selected_scene_node) {
                        selected_scene_node = nullptr;
                    }
                    delete *it;
                    it = nodes.erase(it);
                } else {
                    it++;
                }
            }

            ImGui::EndChild();
            ImGui::PopStyleVar();

            ImGui::EndTabItem();
        }
        //if (ImGui::BeginTabItem("Debugger")) {
        //    bool msaa_enabled = Renderer::instance->get_msaa_count() != 1;

        //    if (ImGui::Checkbox("Enable MSAAx4", &msaa_enabled)) {
        //        if (msaa_enabled) {
        //            Renderer::instance->set_msaa_count(4);
        //        } else {
        //            Renderer::instance->set_msaa_count(1);
        //        }
        //    }

        //    bool pause_frustum_culling_camera = Renderer::instance->get_frustum_camera_paused();

        //    if (ImGui::Checkbox("Pause frustum culling camera", &pause_frustum_culling_camera)) {
        //        Renderer::instance->set_frustum_camera_paused(pause_frustum_culling_camera);
        //    }

        //    ImGui::EndTabItem();
        //}
        //ImGui::EndTabBar();
    }

    ImGui::Separator();

    if (selected_scene_node && scene_tab_open) {
        ImGui::BeginChild("NodeProperties", ImVec2(0, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_Borders, ImGuiWindowFlags_None);

        selected_scene_node->render_gui();

        ImGui::EndChild();
    }

    ImGui::End();
}

bool DebugManager::render_scene_tree_recursive(Node* entity)
{
    std::vector<Node*>& children = entity->get_children();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (entity == selected_scene_node) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (ImGui::TreeNodeEx(entity->get_name().c_str(), flags)) {
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            if (selected_scene_node != entity) {
                selected_scene_node = entity;
            } else {
                selected_scene_node = nullptr;
            }
        }

        if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
        {
            if (ImGui::Button("Delete")) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                ImGui::TreePop();
                selected_scene_node = nullptr;
                Node::emit_signal("@node_deleted", (void*)entity);
                return true;
            }
            ImGui::EndPopup();
        }

        std::vector<Node*>::iterator it = children.begin();

        while (it != children.end()) {
            if (render_scene_tree_recursive(*it)) {
                if (*it == selected_scene_node) {
                    selected_scene_node = nullptr;
                }

                delete *it;
                it = children.erase(it);
            } else {
                it++;
            }
        }

        ImGui::TreePop();
    }

    return false;
}
