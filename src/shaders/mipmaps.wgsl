@group(0) @binding(0) var previousMipLevel: texture_2d<f32>;

#ifdef RGBA8_UNORM
@group(0) @binding(1) var nextMipLevel: texture_storage_2d<rgba8unorm, write>;
#endif

#ifdef RGBA32_FLOAT
@group(0) @binding(1) var nextMipLevel: texture_storage_2d<rgba32float, write>;
#endif

@group(0) @binding(2) var texture_sampler : sampler;

// https://www.gamedev.net/forums/topic/709862-downsampling-image-in-compute-shader/
@compute @workgroup_size(8, 8)
fn compute(@builtin(global_invocation_id) id: vec3<u32>) {

    let dim : vec2u = textureDimensions(nextMipLevel).xy;

    let uv : vec2f = (vec2f(id.xy) + vec2f(0.5)) / vec2f(dim);
    let color : vec4f = textureSampleLevel(previousMipLevel, texture_sampler, uv, 0.0f);

    textureStore(nextMipLevel, id.xy, color);
}