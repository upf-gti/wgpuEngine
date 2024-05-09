struct VertexInput {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
    @location(2) normal: vec3f,
    @location(3) tangent: vec3f,
    @location(4) color: vec3f,
    @location(5) weights: vec4f,
    @location(6) joints: vec4i
};
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

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

    let xr_image = textureSample(left_eye_texture, texture_sampler, in.uv);

    var out: FragmentOutput;
    out.color = vec4f(pow(xr_image.rgb, 1.0 / vec3f(2.2)), 1.0); // Color

    return out;
}
