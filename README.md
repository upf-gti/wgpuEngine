An open-source framework that allows creating desktop, XR and web 3D applications. Still in very early stages.

It uses Dawn, which provides WebGPU for desktop and web (via emscripten). We are using a [forked version](https://github.com/blitz-research/dawn) with XR support, not the official one (until support is added).

For desktop XR it uses OpenXR. XR on the web still not possible until WebGPU and WebXR are integrated.

## How To Build:
For desktop:
```
git submodule update --init --recursive
mkdir build
cd build
cmake ..
```
For Web:


Download [emscripten](https://emscripten.org/) and follow installation guide.


On Window you may need to download [ninja](https://ninja-build.org/) and include the folder in your PATH environment variable, then:


```
git submodule update --init --recursive
mkdir build-web
cd build-web
emcmake cmake ..
cmake --build .
```
