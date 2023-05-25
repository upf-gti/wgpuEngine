#pragma once

namespace RAW_SHADERS {
    const char simple_shaders[] = R"(

@group(0) @binding(0) var<uniform> uViewProjection: mat4x4f;

@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
    var p = vec4<f32>(0.0, 0.0, -2.0, 1.0);
    if (in_vertex_index == 0u) {
        p = vec4<f32>(-0.5, -0.5, -2.0, 1.0);
    } else if (in_vertex_index == 1u) {
        p = vec4<f32>(0.5, -0.5, -2.0, 1.0);
    } else {
        p = vec4<f32>(0.0, 0.5, -2.0, 1.0);
    }
    return uViewProjection * p;
}

@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(0.0, 0.4, 1.0, 1.0);
})";

};