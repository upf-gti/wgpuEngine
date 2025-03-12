#include "engine.h"

#include "framework/input.h"
#include "framework/utils/file_watcher.h"
#include "framework/ui/io.h"
#include "framework/utils/tinyfiledialogs.h"
#include "framework/nodes/mesh_instance_3d.h"
#include "framework/parsers/parse_scene.h"

#include "graphics/renderer_storage.h"
#include "graphics/renderer.h"

#if defined(OPENXR_SUPPORT)
#include "xr/openxr/openxr_context.h"
#elif defined(WEBXR_SUPPORT)
#include "xr/webxr/webxr_context.h"
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_wgpu.h"
#include "backends/imgui_impl_glfw.h"

#include "framework/utils/ImGuizmo.h"

#include "framework/parsers/parser.h"

#include "engine/scene.h"

#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>

EM_JS(void, on_engine_pre_initialized, (), {
    onEnginePreInitialized();
});

EM_JS(void, on_engine_initialized, (), {
    onEngineInitialized();
});

EM_JS(void, on_end_frame, (), {
    if (Module.Engine.onFrame) {
        Module.Engine.onFrame();
    }
});

EM_JS(int, canvas_get_width, (), {
  return canvas.clientWidth;
});

EM_JS(int, canvas_get_height, (), {
  return canvas.clientHeight;
});

static EM_BOOL on_web_display_size_changed(int event_type,
    const EmscriptenUiEvent* ui_event, void* user_data)
{
    Engine* engine = reinterpret_cast<Engine*>(user_data);
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
}

Engine::~Engine()
{
    if (use_glfw) {
        glfwTerminate();
    }
}

int Engine::initialize(Renderer* renderer, sEngineConfiguration configuration)
{
    spdlog::set_pattern("[%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);

#ifdef __EMSCRIPTEN__
    on_engine_pre_initialized();
#endif

    renderer->set_msaa_count(configuration.msaa_count, true);

    this->renderer = renderer;
    this->configuration = configuration;

    IO::initialize();

    current_time = glfwGetTime();

    init_shader_watchers();

    pre_initialize_renderer();

    spdlog::info("Engine initialized");

#ifdef __EMSCRIPTEN__

    spdlog::info("Initializing Renderer and WASM module...");

    while (initialize_step() || !wasm_module_initialized) {
        renderer->process_events();
        emscripten_sleep(1); // Allows browser to run events
    }

    spdlog::info("WASM module initialized");

#else

    while (initialize_step()) {
        renderer->process_events();
    }

#endif

    // initialize completed

    renderer->post_initialize();

    init_imgui(renderer->get_glfw_window());

    renderer->set_camera_type(configuration.camera_type);

    post_initialize();

#ifdef __EMSCRIPTEN__
    on_engine_initialized();
#endif

    // submit any initialization commands
    renderer->submit_global_command_encoder();

    return 0;
}

int Engine::post_initialize()
{
    return 0;
}

bool Engine::pre_initialize_renderer()
{
#ifdef __EMSCRIPTEN__
    int screen_width = canvas_get_width();
    int screen_height = canvas_get_height();

    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW,
        (void*)this, 0, on_web_display_size_changed
    );
#else
    int screen_width = configuration.window_width;
    int screen_height = configuration.window_height;
#endif

    const bool use_xr = renderer->get_xr_available();
    const bool use_mirror_screen = get_use_mirror_window();

    if (use_xr && !renderer->get_use_custom_mirror()) {
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

        WebGPUContext* webgpu_context = renderer->get_webgpu_context();

        if (configuration.fullscreen) {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            webgpu_context->screen_width = mode->width;
            webgpu_context->screen_height = mode->height;

            window = glfwCreateWindow(mode->width, mode->height, "ROOMS", monitor, nullptr);
        } else {
            webgpu_context->screen_width = screen_width;
            webgpu_context->screen_height = screen_height;

            window = glfwCreateWindow(screen_width, screen_height, "ROOMS", nullptr, nullptr);
        }


        glfwSetWindowTitle(window, configuration.window_title.c_str());
    }


    Input::init(window, use_mirror_screen, use_glfw);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, resize_callback);

    renderer->pre_initialize(window, use_mirror_screen);

    return 0;
}

bool Engine::initialize_step()
{
    return renderer->initialize();
}

void Engine::init_imgui(GLFWwindow* window)
{
    // Init imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
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
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
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
    Node2D::clean();

    if (main_scene) {
        delete main_scene;
    }

    renderer->clean();
}

void Engine::start_loop()
{
    spdlog::info("Loop started");

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(
        [](void* user_data) {
            Engine* engine = reinterpret_cast<Engine*>(user_data);
            engine->on_frame();
        },
        (void*)this,
        0, true
    );

#else
    while (!stop_game_loop) {

        on_frame();

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
    return renderer->get_xr_available();
}

bool Engine::get_use_mirror_window()
{
#ifdef USE_MIRROR_WINDOW
    return true;
#else
    return false;
#endif
}

bool Engine::should_close()
{
    return stop_game_loop;
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

void Engine::get_scene_ray(glm::vec3& ray_origin, glm::vec3& ray_direction)
{
    if (renderer->get_xr_available()) {
        ray_origin = Input::get_controller_position(HAND_RIGHT, POSE_AIM);
        glm::mat4x4 select_hand_pose = Input::get_controller_pose(HAND_RIGHT, POSE_AIM);
        ray_direction = get_front(select_hand_pose);
    }
    else {
        Camera* camera = renderer->get_camera();
        glm::vec3 ray_dir = camera->screen_to_ray(Input::get_mouse_position());
        ray_origin = camera->get_eye();
        ray_direction = glm::normalize(ray_dir);
    }
}

void Engine::on_frame()
{
    renderer->process_events();

    // Update stuff

    Parser::poll_async_parsers();

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

    render();

#ifdef __EMSCRIPTEN__
    on_end_frame();
#endif

    Input::set_mouse_wheel(0.0f, 0.0f);
    Input::set_prev_state();

    IO::end_frame();

    renderer->increase_frame_counter();
}

void Engine::update(float delta_time)
{
#ifndef NDEBUG
    shader_reload_watcher->update(delta_time);
    engine_shader_reload_watcher->update(delta_time);
#endif

    if (Input::was_key_pressed(GLFW_KEY_G)) {
        show_imgui = !show_imgui;
    }

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
            if (ImGui::MenuItem("Open scene (.gltf, .glb, .obj, .vdb, .ply)"))
            {
                std::vector<const char*> filter_patterns = { "*.gltf", "*.glb", "*.obj", "*.vdb", "*.ply" };
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

            if (ImGui::MenuItem("Exit")) {
                exit(0);
            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
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

    float right_panel_width = 350.0f; // px
    float right_panel_height = 570.0f; // px
    WebGPUContext* webgpu_context = Renderer::instance->get_webgpu_context();

    ImGui::SetNextWindowPos({ static_cast<float>(webgpu_context->screen_width) - right_panel_width, 18.0f });
    ImGui::SetNextWindowSize({ right_panel_width, right_panel_height });
    ImGui::Begin("Debug panel", &active, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    bool scene_tab_open = false;
    if (ImGui::BeginTabBar("TabBar", tab_bar_flags))
    {
        scene_tab_open = ImGui::BeginTabItem("Scene");
        if (scene_tab_open)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
            ImGui::BeginChild("SceneTree", ImVec2(0, 260), ImGuiChildFlags_Border, ImGuiWindowFlags_None);

            std::vector<Node*>& nodes = main_scene->get_nodes();
            std::vector<Node*>::iterator it = nodes.begin();
            while (it != nodes.end())
            {
                if (render_scene_tree_recursive(*it)) {
                    if (*it == selected_node) {
                        selected_node = nullptr;
                    }
                    delete* it;
                    it = nodes.erase(it);
                }
                else {
                    it++;
                }
            }

            ImGui::EndChild();
            ImGui::PopStyleVar();

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


    if (selected_node && scene_tab_open) {
        ImGui::BeginChild("NodeProperties", ImVec2(0, 260), ImGuiChildFlags_Border, ImGuiWindowFlags_None);

        selected_node->render_gui();

        ImGui::EndChild();
    }


    ImGui::End();
}

bool Engine::render_scene_tree_recursive(Node* entity)
{
    std::vector<Node*>& children = entity->get_children();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (entity == selected_node) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (ImGui::TreeNodeEx(entity->get_name().c_str(), flags))
    {

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            if (selected_node != entity) {
                selected_node = entity;
            }
            else {
                selected_node = nullptr;
            }
        }

        if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
        {
            if (ImGui::Button("Delete")) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                ImGui::TreePop();
                selected_node = nullptr;
                Node::emit_signal("@node_deleted", (void*)entity);
                return true;
            }
            ImGui::EndPopup();
        }

        std::vector<Node*>::iterator it = children.begin();

        while (it != children.end())
        {
            if (render_scene_tree_recursive(*it)) {

                if (*it == selected_node) {
                    selected_node = nullptr;
                }

                delete *it;
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
    if (!renderer->get_xr_available()) {
        ImGui_ImplWGPU_InvalidateDeviceObjects();
        renderer->resize_window(width, height);
        ImGui_ImplWGPU_CreateDeviceObjects();
    } else {
        renderer->resize_window(width, height);
    }
}

void Engine::vibrate_hand(int controller, float amplitude, float duration)
{
#ifdef OPENXR_SUPPORT
    if (renderer->get_xr_available()) {
        OpenXRContext* openxr_context = static_cast<OpenXRContext*>(renderer->get_xr_context());
        openxr_context->apply_haptics(controller, amplitude, duration);
    }
#endif
}
