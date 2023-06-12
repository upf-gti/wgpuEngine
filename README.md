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
