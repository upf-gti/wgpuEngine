#include mesh_includes.wgsl

@group(0) @binding(0) var source_texture: texture_2d<f32>;
@group(0) @binding(1) var source_sampler : sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(in.position, 1.0);
    out.uv = in.uv;
    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {
    var out: FragmentOutput;
    out.color = textureSample(source_texture, source_sampler, in.uv);

#ifdef APPLY_GAMMA
    out.color = vec4f(pow(out.color.rgb, 1.0 / vec3f(2.2)), 1.0);
#endif

    return out;
}
