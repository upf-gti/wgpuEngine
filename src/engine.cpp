#include "engine.h"
#include "utils.h"
#include "framework/input.h"
#include "framework/file_watcher.h"

int Engine::initialize(Renderer* renderer, GLFWwindow* window, bool use_mirror_screen)
{
    shader_reload_watcher = new FileWatcher("./data/shaders/", 1.0f, [](std::string path_to_watch, eFileStatus status) -> void {

        // Process only regular files, all other file types are ignored
        if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != eFileStatus::Erased) {
            return;
        }

        // Remove "./" from path
        path_to_watch.erase(0, 2);

        switch (status) {
        case eFileStatus::Modified: {
            std::cout << "Shader modified: " << path_to_watch << '\n';
            Shader* shader = Shader::get(path_to_watch);
            if (shader) {
                shader->reload();
            }
            break;
        }
        default:
            std::cout << "Error! Unknown file status.\n";
        }
    });

    this->renderer = renderer;

    if(!renderer->initialize(window, use_mirror_screen)) {
        Input::init(window, renderer);
        return 0;
    }

    return 1;
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
