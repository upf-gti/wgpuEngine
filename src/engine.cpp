#include "engine.h"

#include "utils.h"
#include "framework/input.h"

int Engine::initialize(Renderer* renderer, GLFWwindow* window, bool use_mirror_screen)
{
    Input::init(window, use_mirror_screen);
    this->renderer = renderer;
    return renderer->initialize(window, use_mirror_screen);
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

    renderer->update(delta_time);
}

void Engine::render()
{
    renderer->render();
}
