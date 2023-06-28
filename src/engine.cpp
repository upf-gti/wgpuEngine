#include "engine.h"
#include "framework/input.h"
#include "utils.h"

int Engine::initialize(Renderer* renderer, GLFWwindow* window, bool use_mirror_screen)
{
    this->renderer = renderer;

    if(  !renderer->initialize(window, use_mirror_screen) ) {
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

void Engine::update(double delta_time)
{
    renderer->update(delta_time);

    Input::update(delta_time);
}

void Engine::render()
{
    renderer->render();
}
