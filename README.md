# wgpuEngine: a Cross-Platform, WebGPU-based 3D Engine

`wgpuEngine` is an open-source cross-platform engine designed for creating desktop, XR (extended reality), and 3D web applications. Built on top of the newest graphics API WebGPU, it provides a solution to render complex scenes, animations, and immersive experiences within web browsers. 

It uses Dawn, which provides WebGPU for desktop and web (via [emscripten](https://emscripten.org/)). We are still using a [forked version](https://github.com/blitz-research/dawn) until the official one adds XR support. It supports XR via OpenXR for desktop and WebXR for web environments.

> [!IMPORTANT]
> You can also read our most recent publication ***"A Cross-Platform, WebGPU-Based 3D Engine for Real-Time Rendering and XR Applications"***, from the *Web3D '25: Proceedings of the 30th International Conference on 3D Web Technology*.
[doi/10.1145/3746237.3746305](https://dl.acm.org/doi/10.1145/3746237.3746305)

glTF scene showcasing PBR  |  Gaussian Splatting scan (by [FABW](https://animationsinstitut.de/de/))
:-------------------------:|:-------------------------:
![image](https://github.com/user-attachments/assets/5a190d79-402e-4f69-a7b6-fb505e1488cb)  |  ![image](https://github.com/user-attachments/assets/7b472234-a941-4095-b082-bc5099b0ac1a)

## Documentation & Examples

The documentation is still in progress, but you can find it [here](https://upf-gti.github.io/wgpuEngine/)!
For an example of how to use this engine, check out [Rooms](https://github.com/upf-gti/rooms)!

## Features

- Web export
- Flat screen + VR Rendering Supported
  - Support for Web XR is already available via new WebGPU-WebXR binding!
- Supported platforms:
    - **Windows**
    - **Web Browsers**
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

## Experimental

- C++ to JavaScript bindings to build web applications on the web from scratch (see [live web editor](https://upf-gti.github.io/wgpuEngineSample/))
- Support for web XR applications using brand new WebGPU-WebXR interoperability!
- Support for deferred rendering

## Roadmap

- Shadow mapping
- Android support
- Physics integration

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

Next step is to add the engine as a submodule of your project and initialize the submodules::

```bash
git submodule update --init --recursive
```

### Desktop

Create the build folder and run the **CMakeLists.txt** file using CMake:

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

This project has received partial financial support of:

|  MAX-R Project (HORIZON) | Wi-XR Project (PID2021-123995NB-I00) |
| --- | --- |
| ![logomaxr](./docs/images/logo_maxr_main_sRGB.png#gh-light-mode-only) ![logomaxr](./docs/images/logo_maxr_main_sRGB_light.png#gh-dark-mode-only) | ![miciu](./docs/images/miciu-cofinanciadoUE-aei.png) |
