#include "engine/engine.h"
#include "graphics/renderer.h"

extern sEngineConfiguration app_config(void);

int main()
{
    sEngineConfiguration configuration = app_config();

    Engine* engine;
    if (configuration.custom_engine_instance) {
        engine = static_cast<Engine*>(configuration.custom_engine_instance);
    }
    else {
        engine = new Engine();
    }

    Renderer* renderer;
    if (configuration.custom_renderer_instance) {
        renderer = static_cast<Renderer*>(configuration.custom_renderer_instance);
    }
    else {
        renderer = new Renderer();
    }

    if (engine->initialize(renderer, configuration)) {
        return 1;
    }

    engine->start_loop();

    engine->clean();

    delete engine;

    return 0;
}
