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
