# wgpuEngine

`wgpuEngine` is an open-source cross-platform engine designed for creating desktop, XR (extended reality), and 3D web applications. Built on top of the newest graphics API  WebGPU, it provides a solution to render complex scenes, animations, and immersive experiences. 

It uses Dawn, which provides WebGPU for desktop and web (via [emscripten](https://emscripten.org/)). We are still using a [forked version](https://github.com/blitz-research/dawn) until the official one adds XR support. For desktop XR it uses OpenXR. 

> [!IMPORTANT]
> Web XR still not available until WebGPU and WebXR are integrated.

The documentation is still in progress, but you can find it [here](https://upf-gti.github.io/wgpuEngine/)!
For an example of how to use this engine, check out [Rooms](https://github.com/upf-gti/rooms)!

## Features

- Web export (Still no XR enabled)
- Flat screen + Desktop VR Rendering Supported
- Supported platforms:
    - **Windows**
    - **Web Chrome**
    - **Mac OS**
- Supported formats:
    - **.obj** (using [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader))
    - **.gltf**/**.glb** (using [tinygltf](https://github.com/syoyo/tinygltf))
    - **.ply** (using [happly](https://github.com/nmwsharp/happly))
    - **.vdb** (using [easyVDB](https://github.com/victorubieto/easyVDB))
- Rendering features:
    - XR Rendering
    - Physically Based Materials (PBR)
    - HDR + IBL Lighting
    - Instancing
    - Gaussian Splatting Renderer
- Scene node management
- Support for Rigid and Skeletal animations
- UI Features:
    - 2D and 3D User Interface
    - 3D Text Rendering
    - 2D and 3D Gizmo ([ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) for 2D)

## Roadmap

- Shadow mapping
- Support deferred rendering
- Android support
- Physics integration
- Following expected WebGPU-WebXR interoperability, support web XR applications

## Quick start

```c++
int main()
{
    Engine* engine = new Engine();

    Renderer* renderer = new Renderer();

    if (engine->initialize(renderer)) {
        return 1;
    }

    engine->start_loop();

    engine->clean();

    delete engine;

    delete renderer;

    return 0;
}
```

To start creating your application, override the `Engine` class! You can also override the Renderer class to customize the following optional render passes:

- custom_pre_opaque_pass, custom_post_opaque_pass
- custom_pre_transparent_pass, custom_post_transparent_pass
- custom_pre_2d_pass, custom_post_2d_pass

### Engine configuration

Customize engine parameters using `sEngineConfiguration` when calling `Engine::initialize`:

```c++
sEngineConfiguration engine_config = {
    .window_title = "Application",
    .fullscren = false
};

if (engine->initialize(renderer, engine_config)) {
    return 1;
}
```

### Renderer configuration

Customize also renderer parameters using `sRendererConfiguration` when instancing your renderer to modify WebGPU context parameters. E.g. required limits or features:

```c++
sRendererConfiguration render_config;

render_config.required_limits.limits.maxStorageBuffersPerShaderStage = 8;
render_config.required_limits.limits.maxComputeInvocationsPerWorkgroup = 1024;

Renderer* renderer = new Renderer(render_config);
```

## How to build

You will need to install the following tools:

- [Git](https://git-scm.com/)
- [CMake](https://cmake.org/download/)
- [Python](https://www.python.org/) (added to your PATH)
- [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) (for Dawn native builds)

and clone the Github repository, also initializing the submodules:

```bash
git clone https://github.com/upf-gti/rooms.git
git submodule update --init --recursive
```

### Desktop

```bash
mkdir build
cd build
cmake ..
```

### Web

Download [emscripten](https://emscripten.org/) and follow the installation guide.

On Windows you may need to download [ninja](https://ninja-build.org/) and include the folder in your PATH environment variable, then:


```bash
mkdir build-web
cd build-web
emcmake cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Support

This project is being developed with partial financial support of:

|  MAX-R Project (HORIZON) | Wi-XR Project (PID2021-123995NB-I00) |
| --- | --- |
| ![logomaxr](./docs/images/logo_maxr_main_sRGB.png#gh-light-mode-only) ![logomaxr](./docs/images/logo_maxr_main_sRGB_light.png#gh-dark-mode-only) | ![miciu](./docs/images/miciu-cofinanciadoUE-aei.png) |
