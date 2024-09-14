#include "engine.h"

#include "framework/input.h"
#include "framework/utils/file_watcher.h"
#include "framework/ui/io.h"
#include "framework/nodes/default_node_factory.h"
#include "framework/utils/tinyfiledialogs.h"
#include "framework/nodes/mesh_instance_3d.h"
#include "framework/scene/parse_scene.h"

#include "graphics/renderer_storage.h"
#include "graphics/renderer.h"

#include "xr/openxr_context.h"

#include "imgui.h"
#include "backends/imgui_impl_wgpu.h"
#include "backends/imgui_impl_glfw.h"

#include "framework/utils/ImGuizmo.h"

#include "engine/scene.h"

#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
EM_JS(int, canvas_get_width, (), {
  return canvas.clientWidth;
    });
EM_JS(int, canvas_get_height, (), {
  return canvas.clientHeight;
    });
static EM_BOOL on_web_display_size_changed(int event_type,
    const EmscriptenUiEvent* ui_event, void* user_data)
{
    RoomsEngine* engine = reinterpret_cast<RoomsEngine*>(user_data);
    engine->resize_window(ui_event->windowInnerWidth, ui_event->windowInnerHeight);
    return true;
}
#endif

Engine* Engine::instance = nullptr;

void resize_callback(GLFWwindow* window, int width, int height)
{
    Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));

    // Minimized window
    if (width == 0 && height == 0) {
        return;
    }

    if (!engine->get_openxr_available()) {
        engine->resize_window(width, height);
    }
}

Engine::Engine()
{
    instance = this;

    node_factory = default_node_factory;
}

Engine::~Engine()
{
    if (use_glfw) {
        glfwTerminate();
    }

    if (main_scene) {
        delete main_scene;
    }
}

int Engine::initialize(Renderer* renderer, sEngineConfiguration configuration)
{
    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);

#ifdef __EMSCRIPTEN__
    int screen_width = canvas_get_width();
    int screen_height = canvas_get_height();

    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,
        (void*)engine, 0, on_web_display_size_changed
    );
#else
    int screen_width = configuration.window_width;
    int screen_height = configuration.window_height;
#endif

    const bool use_xr = renderer->get_openxr_available();
    const bool use_mirror_screen = get_use_mirror_window();

    if (use_xr) {
        // Keep XR aspect ratio
        screen_width = 992;
        screen_height = 1000;
    }

    // Only init glfw if no xr or using mirror
    this->use_glfw = !use_xr || (use_xr && use_mirror_screen);

    GLFWwindow* window = nullptr;

    if (use_glfw) {
        if (!glfwInit()) {
            spdlog::error("Could not initialize GLFW");
            return 1;
        }

        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(screen_width, screen_height, "ROOMS", NULL, NULL);
    }

    this->renderer = renderer;

    if(renderer->initialize(window, use_mirror_screen)) {
        renderer->get_webgpu_context()->close_window();
        spdlog::error("Could not initialize renderer");
        return 1;
    }

    Input::init(window, renderer, use_glfw);
    IO::initialize();

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, resize_callback);

    current_time = glfwGetTime();

    init_imgui(window);
    init_shader_watchers();

    spdlog::info("Engine initialized");

    return 0;
}

void Engine::init_imgui(GLFWwindow* window)
{
    // Init imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOther(window, true);

    ImGui_ImplWGPU_InitInfo init_info = {};
    init_info.Device = renderer->get_webgpu_context()->device;
    init_info.RenderTargetFormat = WGPUTextureFormat_BGRA8Unorm;
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    init_info.NumFramesInFlight = 3;

    ImGui_ImplWGPU_Init(&init_info);

    // Disable file-system access in web builds (don't load imgui.ini)
#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif

}

void Engine::init_shader_watchers()
{
    std::string engine_shaders = WGPUENGINE_PATH + std::string("/data/shaders/");

#ifndef NDEBUG
    shader_reload_watcher = new FileWatcher({ "./data/shaders/" }, 1.0f, [](std::string path_to_watch, eFileStatus status) -> void {

        // Process only regular files, all other file types are ignored
        if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != eFileStatus::Erased) {
            return;
        }

        switch (status) {
        case eFileStatus::Modified: {
            spdlog::info("Shader modified: {}", path_to_watch);
            RendererStorage::reload_shader(path_to_watch);
            break;
        }
        default:
            spdlog::error("Shader reload: Unknown file status");
        }
        });

    engine_shader_reload_watcher = new FileWatcher({ engine_shaders }, 1.0f, [](std::string path_to_watch, eFileStatus status) -> void {

        // Process only regular files, all other file types are ignored
        if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != eFileStatus::Erased) {
            return;
        }

        switch (status) {
        case eFileStatus::Modified: {
            spdlog::info("Shader modified: {}", path_to_watch);
            RendererStorage::reload_engine_shader(path_to_watch);
            break;
        }
        default:
            spdlog::error("Shader reload: Unknown file status");
        }
        });
#endif
}

void Engine::clean()
{
    renderer->clean();

    Node2D::clean();
}

void Engine::start_loop()
{
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(
        [](void* userData) {
            Engine* engine = reinterpret_cast<Engine*>(userData);
            engine->on_frame();
        },
        (void*)this,
        0, true
    );

#else
    while (!stop_game_loop) {
        on_frame();

        std::string fps = "FPS: " + std::to_string(1.0 / (std::max(get_delta_time(), 0.0001f)));
        glfwSetWindowTitle(renderer->get_glfw_window(), fps.c_str());

        stop_game_loop = glfwWindowShouldClose(renderer->get_glfw_window());
    }
#endif
}

void Engine::add_node(Node* node)
{
    main_scene->add_node(node);
}

bool Engine::get_openxr_available()
{
    return renderer->get_openxr_available();
}

bool Engine::get_use_mirror_window()
{
#ifdef USE_MIRROR_WINDOW
    return true;
#else
    return false;
#endif
}

void Engine::set_main_scene(const std::string& scene_path)
{
    if (main_scene) {
        delete main_scene;
    }

    main_scene = new Scene();
    main_scene->parse(scene_path);
}

Scene* Engine::get_main_scene()
{
    return main_scene;
}

void Engine::on_frame()
{
    // Update stuff

    Input::update(delta_time);

    double last_time = current_time;
    current_time = glfwGetTime();
    delta_time = static_cast<float>((current_time - last_time));

    IO::start_frame();

    update(delta_time);

    // Update IO after updating the engine
    IO::update(delta_time);

    // Render stuff

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();

    IO::render();

    render();

    Input::set_mouse_wheel(0.0f, 0.0f);
    Input::set_prev_state();
}

void Engine::update(float delta_time)
{
#ifndef NDEBUG
    shader_reload_watcher->update(delta_time);
    engine_shader_reload_watcher->update(delta_time);
#endif

    renderer->update(delta_time);
}

void Engine::render()
{
    renderer->render();
}

void Engine::render_default_gui()
{
    bool active = true;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open room (.room)"))
            {
                std::vector<const char*> filter_patterns = { "*.room" };
                char const* open_file_name = tinyfd_openFileDialog(
                    "Room loader",
                    "",
                    filter_patterns.size(),
                    filter_patterns.data(),
                    "Rooms format",
                    0
                );

                if (open_file_name) {
                    set_main_scene(open_file_name);
                }
            }
            if (ImGui::MenuItem("Save room (.room)"))
            {
                std::vector<const char*> filter_patterns = { "*.room" };

                char const* save_file_name = tinyfd_saveFileDialog(
                    "Room loader",
                    "",
                    filter_patterns.size(),
                    filter_patterns.data(),
                    "Rooms format"
                );

                if (save_file_name) {
                    main_scene->serialize(save_file_name);
                }
            }
            if (ImGui::MenuItem("Open scene (.gltf, .glb, .obj, .vdb)"))
            {
                std::vector<const char*> filter_patterns = { "*.gltf", "*.glb", "*.obj", "*.vdb"};
                char const* open_file_name = tinyfd_openFileDialog(
                    "Scene loader",
                    "",
                    filter_patterns.size(),
                    filter_patterns.data(),
                    "Scene formats",
                    0
                );

                if (open_file_name) {
                    std::vector<Node*> entities;
                    parse_scene(open_file_name, entities);
                    main_scene->add_nodes(entities);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // ImGui::SetNextWindowSize({ 300, 400 });
    ImGui::Begin("Debug panel", &active, ImGuiWindowFlags_NoFocusOnAppearing);

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("TabBar", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Scene"))
        {
            if (ImGui::TreeNodeEx("Root", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
                {
                    if (ImGui::Button("Delete All")) {
                        main_scene->delete_all();
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                std::vector<Node*>& nodes = main_scene->get_nodes();
                std::vector<Node*>::iterator it = nodes.begin();
                while (it != nodes.end())
                {
                    if (render_scene_tree_recursive(*it)) {
                        delete* it;
                        it = nodes.erase(it);
                    }
                    else {
                        it++;
                    }
                }

                ImGui::TreePop();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Debugger"))
        {
            bool msaa_enabled = Renderer::instance->get_msaa_count() != 1;

            if (ImGui::Checkbox("Enable MSAAx4", &msaa_enabled)) {
                if (msaa_enabled) {
                    Renderer::instance->set_msaa_count(4);
                }
                else {
                    Renderer::instance->set_msaa_count(1);
                }
            }

            bool pause_frustum_culling_camera = Renderer::instance->get_frustum_camera_paused();

            if (ImGui::Checkbox("Pause frustum culling camera", &pause_frustum_culling_camera)) {
                Renderer::instance->set_frustum_camera_paused(pause_frustum_culling_camera);
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();

    ImGui::End();
}

bool Engine::render_scene_tree_recursive(Node* entity)
{
    std::vector<Node*>& children = entity->get_children();

    MeshInstance3D* entity_mesh = dynamic_cast<MeshInstance3D*>(entity);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;

    if ((entity_mesh && children.empty() && entity_mesh->get_surfaces().empty())) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (ImGui::TreeNodeEx(entity->get_name().c_str(), flags))
    {
        if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
        {
            if (ImGui::Button("Delete")) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                ImGui::TreePop();
                return true;
            }
            ImGui::EndPopup();
        }

        if (entity_mesh) {

            const std::vector<Surface*>& surfaces = entity_mesh->get_surfaces();

            for (int i = 0; i < surfaces.size(); ++i) {
                std::string surface_name = surfaces[i]->get_name();
                std::string final_surface_name = surface_name.empty() ? ("Surface " + std::to_string(i)).c_str() : surface_name;
                ImGui::TreeNodeEx(final_surface_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf);
                ImGui::TreePop();
            }
        }

        entity->render_gui();

        std::vector<Node*>::iterator it = children.begin();

        while (it != children.end())
        {
            if (render_scene_tree_recursive(*it)) {
                delete* it;
                it = children.erase(it);
            }
            else {
                it++;
            }
        }

        ImGui::TreePop();
    }

    return false;
}

void Engine::resize_window(int width, int height)
{
    if (!renderer->get_openxr_available()) {
        ImGui_ImplWGPU_InvalidateDeviceObjects();
        renderer->resize_window(width, height);
        ImGui_ImplWGPU_CreateDeviceObjects();
    } else {
        renderer->resize_window(width, height);
    }
}

void Engine::vibrate_hand(int controller, float amplitude, float duration)
{
#ifdef XR_SUPPORT
    if (renderer->get_openxr_available()) {
        auto openxr_context = renderer->get_openxr_context();
        openxr_context->apply_haptics(controller, amplitude, duration);
    }
#endif
}
