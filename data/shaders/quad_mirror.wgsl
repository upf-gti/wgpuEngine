#include mesh_includes.wgsl

#define GAMMA_CORRECTION

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(in.position, 1.0);
    out.uv = in.uv; // forward to the fragment shader
    return out;
}

@group(0) @binding(0) var left_eye_texture: texture_2d<f32>;
@group(0) @binding(1) var texture_sampler : sampler;

struct FragmentOutput {
    @location(0) color: vec4f
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {

    let uvs : vec2f = vec2f(in.uv.x, in.uv.y);
    let xr_image = textureSample(left_eye_texture, texture_sampler, uvs);

    var out: FragmentOutput;

    if (GAMMA_CORRECTION == 0) {
        out.color = vec4f(pow(xr_image.rgb, 1.0 / vec3f(2.2)), 1.0);
    } else {
        out.color = vec4f(xr_image.rgb, 1.0);
    }

    return out;
}
