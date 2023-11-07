#include "engine.h"
#include "utils.h"
#include "framework/input.h"
#include "framework/file_watcher.h"

#include <iostream>

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

            const std::vector<std::string> shader_paths = RendererStorage::get_shader_for_reload(path_to_watch);

            for (const auto& shader_path : shader_paths) {
                Shader* shader = RendererStorage::get_shader(shader_path);
                if (shader) {
                    shader->reload();
                }
            }

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

    Input::init(window, renderer);

    glfwSetWindowUserPointer(window, this);

    glfwSetFramebufferSizeCallback(window, resize_callback);

    current_time = glfwGetTime();

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

void Engine::on_frame()
{
    if (use_glfw) {
        glfwPollEvents();
    }

    double last_time = current_time;
    current_time = glfwGetTime();

    delta_time = static_cast<float>((current_time - last_time));

    update(delta_time);
    render();
}

void Engine::update(float delta_time)
{
    Input::update(delta_time);

    shader_reload_watcher->update(delta_time);

    renderer->update(delta_time);
}

void Engine::render()
{
    renderer->render();
}

void Engine::resize_window(int width, int height)
{
    renderer->resize_window(width, height);
}
