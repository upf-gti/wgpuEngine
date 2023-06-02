#pragma once

namespace RAW_SHADERS {
    const char simple_shaders[] = R"(

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

@group(0) @binding(0) var left_eye_texture: texture_2d<f32>;
@group(0) @binding(1) var right_eye_texture: texture_2d<f32>;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    var texture_size = textureDimensions(right_eye_texture);
    var uv_flip = in.uv;
    uv_flip.y = 1.0 - uv_flip.y;
    let color = textureLoad(right_eye_texture, vec2u(uv_flip * vec2f(texture_size)) , 0);
    return color;
}

)";

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

    const char compute_shader[] = R"(

@group(0) @binding(0) var left_eye_texture: texture_storage_2d<rgba8unorm,write>;
@group(0) @binding(1) var right_eye_texture: texture_storage_2d<rgba8unorm,write>;

@compute @workgroup_size(16, 16, 1)
fn compute(@builtin(global_invocation_id) id: vec3<u32>) {
    textureStore(left_eye_texture, id.xy, vec4f(1.0, 1.0, 0.0, 1.0));
    textureStore(right_eye_texture, id.xy, vec4f(1.0, 0.0, 0.0, 1.0));
}

)";

};