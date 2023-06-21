An open-source framework that allows creating desktop, XR and web 3D applications. Still in very early stages.

It uses Dawn, which provides WebGPU for desktop and web (via emscripten). We are using a [forked version](https://github.com/blitz-research/dawn) with XR support, not the official one (until support is added).

For desktop XR it uses OpenXR. XR on the web still not possible until WebGPU and WebXR are integrated.

See [Rooms](https://github.com/upf-gti/rooms) for an example of how to use this framework. 