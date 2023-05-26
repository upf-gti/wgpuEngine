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

    const char mirror_shaders[] = R"(

struct VertexInput {
    @location(0) position: vec2f,
    @location(1) uv: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(in.position, 0.0, 1.0);
    out.uv = in.uv; // forward to the fragment shader
    return out;
}

@group(0) @binding(0) var leftEyeTexture: texture_2d<f32>;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    var texture_size = textureDimensions(leftEyeTexture);
    var uv_flip = in.uv;
    uv_flip.y = 1.0 - uv_flip.y;
    let color = textureLoad(leftEyeTexture, vec2u(uv_flip * vec2f(texture_size)) , 0);
    return color;
}

)";
};