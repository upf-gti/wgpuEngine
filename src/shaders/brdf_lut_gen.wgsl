#include pbr_functions.wgsl

@group(0) @binding(0) var brdf_lut: texture_storage_2d<rg32float, write>;

const SAMPLE_COUNT = 1024u;

const TEXTURE_WIDTH = 512.0;
const TEXTURE_HEIGHT = 512.0;

// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/shaders/ibl_filtering.frag#L364
fn integrate_BRDF(NdotV : f32, roughness : f32) -> vec2f
{
    let V : vec3f = vec3f(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);

    let N : vec3f = vec3f(0.0, 0.0, 1.0);

    var A : f32 = 0.0;
    var B : f32 = 0.0;

    let roughness2 : f32 = roughness * roughness;
    let roughness4 : f32 = roughness2 * roughness2;

    for(var i : u32 = 0u; i < SAMPLE_COUNT; i = i + 1u)
    {
        let Xi : vec2f = Hammersley(i, SAMPLE_COUNT);

        let H : vec3f = importance_sample_GGX(Xi, roughness4);

        let L : vec3f = normalize(reflect(-V, H));

        let NdotL : f32 = clamp(L.z, 0.0, 1.0);
        let NdotH : f32 = clamp(H.z, 0.0, 1.0);
        let VdotH : f32 = clamp(dot(V, H), 0.0, 1.0);
        
        if (NdotL > 0.0)
        {
            let V_pdf : f32 = V_SmithGGXCorrelated(NdotV, NdotL, roughness) * VdotH * NdotL / NdotH;
            let Fc : f32 = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * V_pdf;
            B += Fc * V_pdf;
        }
    }

    return 4.0 * vec2(A, B) / f32(SAMPLE_COUNT);
}

@compute @workgroup_size(16, 16, 1)
fn compute(@builtin(global_invocation_id) id: vec3<u32>) 
{
    textureStore(brdf_lut, id.xy, vec4f(integrate_BRDF(f32(id.x) / TEXTURE_WIDTH, f32(id.y) / TEXTURE_HEIGHT), 0.0, 0.0));
}
