#include math.wgsl

@group(0) @binding(0) var input_panorama_texture: texture_2d<f32>;
@group(0) @binding(1) var output_cubemap_texture: texture_storage_2d_array<rgba32float, write>;
@group(0) @binding(2) var texture_sampler : sampler;

// From https://github.com/eliemichel/LearnWebGPU-Code/blob/step220/resources/compute-shader.wgsl

struct CubeMapUVL {
    uv: vec2f,
    layer: u32,
}

fn directionFromCubeMapUVL(uvl: CubeMapUVL) -> vec3f {

    let uvx = 2.0 * uvl.uv.x - 1.0;
    let uvy = 2.0 * uvl.uv.y - 1.0;
    switch (uvl.layer) {
        case 0u {
            return vec3f(1.0, uvy, -uvx);
        }
        case 1u {
            return vec3f(-1.0, uvy, uvx);
        }
        case 2u {
            return vec3f(uvx, -1.0, uvy);
        }
        case 3u {
            return vec3f(uvx, 1.0, -uvy);
        }
        case 4u {
            return vec3f(uvx,  uvy,  1.0);
        }
        case 5u {
            return vec3f(-uvx, uvy, -1.0);
        }
        default {
            return vec3f(0.0); // should not happen
        }
    }
}

fn textureGatherWeights_2df(t: texture_2d<f32>, uv: vec2f) -> vec2f {
    let dim = textureDimensions(t).xy;
    let scaled_uv = uv * vec2f(dim);
    // This is not accurate, see see https://www.reedbeta.com/blog/texture-gathers-and-coordinate-precision/
    // but bottom line is:
    //   "Unfortunately, if we need this to work, there seems to be no option but to check
    //    which hardware you are running on and apply the offset or not accordingly."
    return fract(scaled_uv - 0.5);
}

@compute @workgroup_size(4, 4, 6)
fn compute(@builtin(global_invocation_id) id: vec3<u32>) {
    let input_dimensions = textureDimensions(input_panorama_texture).xy;
    let output_dimensions = textureDimensions(output_cubemap_texture).xy;

    let layer = id.z;

    let uv = vec2f(id.xy) / vec2f(output_dimensions);

    let direction = normalize(directionFromCubeMapUVL(CubeMapUVL(uv, layer)));

    let phi = 0.5 + 0.5 * atan2(-direction.x, direction.z) / M_PI;
    let theta = 1.0 - acos(direction.y) / M_PI;
    let latlong_uv = vec2f(phi, theta);

    let samples = array<vec4f, 4>(
        textureGather(0, input_panorama_texture, texture_sampler, latlong_uv),
        textureGather(1, input_panorama_texture, texture_sampler, latlong_uv),
        textureGather(2, input_panorama_texture, texture_sampler, latlong_uv),
        textureGather(3, input_panorama_texture, texture_sampler, latlong_uv),
    );

    let w = textureGatherWeights_2df(input_panorama_texture, latlong_uv);
    // TODO: could be represented as a matrix/vector product
    let color = vec4f(
        mix(mix(samples[0].w, samples[0].z, w.x), mix(samples[0].x, samples[0].y, w.x), w.y),
        mix(mix(samples[1].w, samples[1].z, w.x), mix(samples[1].x, samples[1].y, w.x), w.y),
        mix(mix(samples[2].w, samples[2].z, w.x), mix(samples[2].x, samples[2].y, w.x), w.y),
        mix(mix(samples[3].w, samples[3].z, w.x), mix(samples[3].x, samples[3].y, w.x), w.y),
    );

    // let new_color = textureLoad(input_panorama_texture, vec2<i32>(latlong_uv * vec2<f32>(input_dimensions)), 0);

    textureStore(output_cubemap_texture, id.xy, layer, color);
}