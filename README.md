# wgpuEngine

wgpuEngine is an open-source framework that allows creating desktop, XR and web 3D applications.

It uses Dawn, which provides WebGPU for desktop and web (via emscripten). We are using a [forked version](https://github.com/blitz-research/dawn) with XR support, not the official one (until support is added).

For desktop XR it uses OpenXR. XR on the web still not possible until WebGPU and WebXR are integrated.

See [Rooms](https://github.com/upf-gti/rooms) for an example of how to use this framework. 

## Features

- Web export (Still no XR enabled)
- Flat screen + Desktop VR Rendering Supported
- OBJ/GLB parsing
- Instancing
- Physically Based Materials (PBR)
- HDR+IBL Lighting
- Scene node management
- 2D/3D UI
- Text Rendering

## How to build

You will need to install the following tools:

- [CMake](https://cmake.org/download/)
- [Python](https://www.python.org/) (added to your PATH)
- [Vulkan SDK](https://vulkan.lunarg.com/)

### Desktop

```bash
git submodule update --init --recursive
mkdir build
cd build
cmake ..
```

### Web


Download [emscripten](https://emscripten.org/) and follow the installation guide.


On Windows you may need to download [ninja](https://ninja-build.org/) and include the folder in your PATH environment variable, then:


```bash
git submodule update --init --recursive
mkdir build-web
cd build-web
emcmake cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```