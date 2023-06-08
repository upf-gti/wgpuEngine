#include "engine.h"

#include "utils.h"

int Engine::initialize(GLFWwindow *window, bool use_mirror_screen)
{
    return renderer.initialize(window, use_mirror_screen);
}

void Engine::clean()
{
    renderer.clean();
}

bool Engine::get_openxr_available()
{
    return renderer.get_openxr_available();
}

bool Engine::get_use_mirror_window()
{
#ifdef USE_MIRROR_WINDOW
    return true;
#else
    return false;
#endif
}

void Engine::render()
{
    renderer.render();
}
