## Dependencies
* wgpu-native's submodules: git submodule update --init --recursive
* clang: scoop install llvm
* Rust (Cargo): scoop install rust

## Installation:
git submodule update --init --recursive
scoop install llvm rust
cd wgpu-native
make lib-native lib-native-release