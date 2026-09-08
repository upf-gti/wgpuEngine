#include "core/managers/debug/debug_manager.h"
#include "core/managers/display/display_manager.h"
#include "core/managers/engine/engine_manager.h"
#include "core/managers/fs/file_system_manager.h"
#include "core/managers/input/input_manager.h"
#include "core/managers/render/render_manager.h"
#include "core/managers/simulation/simulation_manager.h"
#include "core/managers/xr/xr_manager.h"

extern void get_engine_config(sEngineConfig& out_config);

int main(int argc, char** argv)
{
    // Create managers
    SimulationManager simulation_manager;
    RenderManager render_manager;
    DisplayManager display_manager;
    XRManager xr_manager;
    InputManager input_manager;
    FileSystemManager file_system_manager;
    DebugManager debug_manager;
    EngineManager engine_manager;

    engine_manager.initialize();

    sEngineConfig configuration = {};
    get_engine_config(configuration);

    engine_manager.set_configuration(configuration);

    xr_manager.initialize();
    xr_manager.create_xr_instance();

    render_manager.initialize();
    xr_manager.create_xr_context();

    simulation_manager.initialize();
    display_manager.initialize();
    input_manager.initialize();
    file_system_manager.initialize();
    debug_manager.initialize();

    simulation_manager.start_main_loop();

    debug_manager.finalize();
    file_system_manager.finalize();
    input_manager.finalize();
    display_manager.finalize();
    xr_manager.finalize();
    render_manager.finalize();
    simulation_manager.finalize();

    return 0;
}
