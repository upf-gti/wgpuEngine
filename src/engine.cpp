#include "engine.h"

#include "utils.h"

int Engine::initialize(GLFWwindow *window, bool use_mirror_screen) {
    return renderer.initialize(window, use_mirror_screen);
}

void Engine::clean() {
    renderer.clean();
}

bool Engine::isOpenXRAvailable()
{
    return renderer.isOpenXRAvailable();
}

bool Engine::useMirrorWindow()
{
#ifdef USE_MIRROR_WINDOW
    return renderer.isOpenXRAvailable();
#else
    return false;
#endif
}

void Engine::render() {

    renderer.render();
}
