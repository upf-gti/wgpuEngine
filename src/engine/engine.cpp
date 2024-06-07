#include "engine.h"

#include "framework/input.h"
#include "framework/utils/file_watcher.h"
#include "framework/ui/io.h"

#include "graphics/renderer_storage.h"
#include "graphics/renderer.h"

#include "xr/openxr_context.h"

#include "imgui.h"
#include "backends/imgui_impl_wgpu.h"
#include "backends/imgui_impl_glfw.h"
#include "framework/utils/ImGuizmo.h"

#include <GLFW/glfw3.h>

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

int Engine::initialize(Renderer* renderer, GLFWwindow* window, bool use_glfw, bool use_mirror_screen)
{
    this->use_glfw = use_glfw;

    shader_reload_watcher = new FileWatcher("./data/shaders/", 1.0f, [](std::string path_to_watch, eFileStatus status) -> void {

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

    this->renderer = renderer;

    if(renderer->initialize(window, use_mirror_screen)) {
        return 1;
    }

    Input::init(window, renderer, use_glfw);

    IO::initialize();

    glfwSetWindowUserPointer(window, this);

    glfwSetFramebufferSizeCallback(window, resize_callback);

    current_time = glfwGetTime();

    // Init imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOther(window, true);
    ImGui_ImplWGPU_Init(renderer->get_webgpu_context()->device, 3, WGPUTextureFormat_BGRA8Unorm, WGPUTextureFormat_Undefined);
    
    // Disable file-system access in web builds (don't load imgui.ini)
#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif

    return 0;
}

void Engine::clean()
{
    renderer->clean();
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

    IO::end_frame();

    // Render stuff

    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();

    render();

    Input::set_mouse_wheel(0.0f, 0.0f);
    Input::set_prev_state();
}

void Engine::update(float delta_time)
{
    shader_reload_watcher->update(delta_time);

    renderer->update(delta_time);
}

void Engine::render()
{
    renderer->render();
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
    auto openxr_context = renderer->get_openxr_context();
    openxr_context->apply_haptics(controller, amplitude, duration);
#endif
}
