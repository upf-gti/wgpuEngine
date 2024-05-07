#include pbr_functions.wgsl

struct Uniforms {
    current_mip_level: u32,
    mip_level_count: u32,
    pad0: u32,
    pad1: u32,
}

@group(0) @binding(0) var input_cubemap_texture: texture_cube<f32>;
@group(0) @binding(1) var output_cubemap_texture: texture_storage_2d_array<rgba32float, write>;
@group(0) @binding(2) var texture_sampler : sampler;
#dynamic @group(0) @binding(3) var<uniform> uniforms: Uniforms;

struct CubeMapUVL {
    uv: vec2f,
    layer: u32,
}

const SAMPLE_COUNT = 1024u;

// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/shaders/ibl_filtering.frag#L257
// Mipmap Filtered Samples (GPU Gems 3, 20.4)
// https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-20-gpu-based-importance-sampling
// https://cgg.mff.cuni.cz/~jaroslav/papers/2007-sketch-fis/Final_sap_0073.pdf
fn compute_lod(pdf : f32, texture_width : f32) -> f32
{
    let omegaS : f32 = 1.0 / (f32(SAMPLE_COUNT) * pdf);
    let omegaP : f32 = 4.0 * PI / (6.0 * texture_width * texture_width);
    return 0.5 * log2(omegaS / omegaP);

    // // https://cgg.mff.cuni.cz/~jaroslav/papers/2007-sketch-fis/Final_sap_0073.pdf
    // return 0.5 * log2( 6.0 * texture_width * texture_width / (f32(SAMPLE_COUNT) * pdf));
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

fn luma(color : vec3f) -> f32
{
    return dot(color, vec3f(0.299, 0.587, 0.114));
}

// https://github.com/eliemichel/LearnWebGPU-Code/blob/step222/resources/compute-shader.wgsl#L448
// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/shaders/ibl_filtering.frag#L283
@compute @workgroup_size(4, 4, 6)
fn compute(@builtin(global_invocation_id) id: vec3u)
{
    let layer = id.z;
    var color : vec3f = vec3f(0.0, 0.0, 0.0);
    var total_weight = 0.0;

    let roughness = f32(uniforms.current_mip_level) / f32(uniforms.mip_level_count - 1);

    let output_dimensions = textureDimensions(output_cubemap_texture).xy;
    var uv = vec2f(id.xy) / vec2f(output_dimensions - 1u);

    var N = normalize(directionFromCubeMapUVL(CubeMapUVL(uv, layer)));
    N.y *= -1;

    let solid_angle_texel : f32 = 4.0 * PI / (6.0 * f32(output_dimensions.x * output_dimensions.x));
    let roughness2 : f32 = roughness * roughness;
    let roughness4 : f32 = roughness2 * roughness2;
    let UpVector : vec3f = select(vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), abs(N.z) < 0.999);
    var T : mat3x3<f32>;
    T[0] = normalize(cross(UpVector, N));
    T[1] = cross(N, T[0]);
    T[2] = N;

    // let local_to_world = makeLocalFrame(N);

    for(var i : u32 = 0u; i < SAMPLE_COUNT; i = i + 1u)
    {
        let Xi : vec2f = Hammersley(i, SAMPLE_COUNT);

        let H : vec3f = T * importance_sample_GGX(Xi, roughness4);
		let NdotH : f32 = dot(N, H);
        let L : vec3f = (2.0 * NdotH * H - N);

        let NdotL : f32 = clamp(dot(N, L), 0.0, 1.0);

        if (NdotL > 0.0)
        {
			let D : f32 = DistributionGGX(NdotH, roughness4);
            let pdf : f32 = D * NdotH / (4.0 * NdotH) + 0.0001;

            let solid_angle_sample : f32 = 1.0 / (f32(SAMPLE_COUNT) * pdf + 0.0001);

            var mipLevel : f32 = 0.5 * log2(solid_angle_sample / solid_angle_texel);

            if (roughness == 0.0) {
                mipLevel = 0.0;
            }

            let radiance_ortho = textureSampleLevel(input_cubemap_texture, texture_sampler, L, mipLevel).rgb;

            color += radiance_ortho * NdotL;
            total_weight += NdotL;
        }
    }

    if (total_weight != 0.0)
    {
        color /= total_weight;
    }
    else
    {
        color /= f32(SAMPLE_COUNT);
    }

    textureStore(output_cubemap_texture, id.xy, layer, vec4(color, 1.0));
}