#include math.wgsl
#include mesh_includes.wgsl
#include tonemappers.wgsl

#define GAMMA_CORRECTION

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

#ifdef ALBEDO_TEXTURE
@group(2) @binding(0) var albedo_texture: texture_2d<f32>;
#endif

@group(2) @binding(1) var<uniform> albedo: vec4f;

#ifdef USE_SAMPLER
@group(2) @binding(7) var sampler_2d : sampler;

#ifdef USE_UV_TRANSFORMS
@group(2) @binding(12) var<uniform> uv_transform_data : array<mat4x4f, 12>;
#endif

#endif // USE_SAMPLER

#ifdef ALPHA_MASK
@group(2) @binding(9) var<uniform> alpha_cutoff: f32;
#endif

#ifdef USE_SKINNING
@group(2) @binding(10) var<storage, read> animated_matrices: array<mat4x4f>;
@group(2) @binding(11) var<storage, read> inv_bind_matrices: array<mat4x4f>;
#endif

#ifndef UNLIT_MATERIAL

#ifdef METALLIC_ROUGHNESS_TEXTURE
@group(2) @binding(2) var metallic_roughness_texture: texture_2d<f32>;
#endif

@group(2) @binding(3) var<uniform> occlusion_roughness_metallic: vec3f;

#ifdef NORMAL_TEXTURE
@group(2) @binding(4) var normal_texture: texture_2d<f32>;
@group(2) @binding(5) var<uniform> normal_scale: f32;
#endif

#ifdef EMISSIVE_TEXTURE
@group(2) @binding(6) var emissive_texture: texture_2d<f32>;
#endif

@group(2) @binding(8) var<uniform> emissive: vec3f;

#ifdef OCLUSSION_TEXTURE
@group(2) @binding(13) var oclussion_texture: texture_2d<f32>;
#endif

#ifdef CLEARCOAT_MATERIAL
@group(2) @binding(14) var<uniform> clearcoat_data: vec2f;

#ifdef CLEARCOAT_TEXTURE
@group(2) @binding(15) var clearcoat_texture: texture_2d<f32>;
#endif

#ifdef CLEARCOAT_ROUGHNESS_TEXTURE
@group(2) @binding(16) var clearcoat_roughness_texture: texture_2d<f32>;
#endif

#ifdef CLEARCOAT_NORMAL_TEXTURE
@group(2) @binding(17) var clearcoat_normal_texture: texture_2d<f32>;
#endif

#endif // CLEARCOAT_MATERIAL

#ifdef IRIDESCENCE_MATERIAL
@group(2) @binding(18) var<uniform> iridescence_data: vec4f;

#ifdef IRIDESCENCE_TEXTURE
@group(2) @binding(19) var iridescence_texture: texture_2d<f32>;
#endif

#ifdef IRIDESCENCE_THICKNESS_TEXTURE
@group(2) @binding(20) var iridescence_thickness_texture: texture_2d<f32>;
#endif

#endif // IRIDESCENCE_MATERIAL

#ifdef ANISOTROPY_MATERIAL
@group(2) @binding(21) var<uniform> anisotropy_data: vec3f;

#ifdef ANISOTROPY_TEXTURE
@group(2) @binding(22) var anisotropy_texture: texture_2d<f32>;
#endif

#endif // ANISOTROPY_MATERIAL

#ifdef TRANSMISSION_MATERIAL
@group(2) @binding(23) var<uniform> transmission_factor: f32;

#ifdef TRANSMISSION_TEXTURE
@group(2) @binding(24) var transmission_texture: texture_2d<f32>;
#endif

#endif // TRANSMISSION_MATERIAL

#include pbr_material.wgsl
#include pbr_functions.wgsl
#include pbr_light.wgsl

#ifdef IRIDESCENCE_MATERIAL
#include pbr_iridescence.wgsl
#endif

#define MAX_LIGHTS

@group(3) @binding(0) var irradiance_texture: texture_cube<f32>;
@group(3) @binding(1) var brdf_lut_texture: texture_2d<f32>;
@group(3) @binding(2) var sampler_clamp: sampler;
@group(3) @binding(3) var<uniform> lights : array<Light, MAX_LIGHTS>;
@group(3) @binding(4) var<uniform> num_lights : u32;

#endif // UNLIT_MATERIAL

#include mesh_material_info.wgsl

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {

    var position = vec4f(in.position, 1.0);
    var normals = vec4f(in.normal, 0.0);

#ifdef USE_SKINNING
    var skin : mat4x4f = (animated_matrices[u32(in.joints.x)] * inv_bind_matrices[u32(in.joints.x)]) * in.weights.x;
    skin += (animated_matrices[u32(in.joints.y)] * inv_bind_matrices[u32(in.joints.y)]) * in.weights.y;
    skin += (animated_matrices[u32(in.joints.z)] * inv_bind_matrices[u32(in.joints.z)]) * in.weights.z;
    skin += (animated_matrices[u32(in.joints.w)] * inv_bind_matrices[u32(in.joints.w)]) * in.weights.w;
    position = skin * position;
    normals = skin * normals;
#endif

    let instance_data : RenderMeshData = mesh_data.data[in.instance_id];

    var out: VertexOutput;
    var world_position = instance_data.model * position;
    out.world_position = world_position.xyz / world_position.w;
    out.position = camera_data.view_projection * world_position;
    out.uv = in.uv; // forward to the fragment shader
    out.color = vec4f(in.color, 1.0) * albedo;

    // incorrect with scaling/non-uniform scaling
    //out.normal = normalize((instance_data.model * normals).xyz);
    out.normal = normalize(adjoint(instance_data.model) * normals.xyz);

#ifdef HAS_TANGENTS
    let tangent : vec3f = normalize(in.tangent.xyz);
    out.tangent = normalize((instance_data.model * vec4f(tangent, 0.0)).xyz);
    out.bitangent = normalize(cross(out.normal, out.tangent) * in.tangent.w);

#ifdef HAS_NORMAL_UV_TRANSFORM
    out.tangent = normalize((uv_transform_data[NORMAL_UV_TRANSFORM] * vec4f(out.tangent, 0.0)).xyz);
    out.bitangent = normalize((uv_transform_data[NORMAL_UV_TRANSFORM] * vec4f(out.bitangent, 0.0)).xyz);
#endif

#endif

    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f
}

@fragment
fn fs_main(in: VertexOutput, @builtin(front_facing) is_front_facing: bool) -> FragmentOutput {

    var out: FragmentOutput;

    var alpha : f32 = 1.0;
    var albedo_color : vec3f = vec3f(0.0);
    var final_color : vec3f = vec3f(0.0);

#ifdef ALBEDO_TEXTURE
    let albedo_uv : vec2f = get_albedo_uv(in.uv);
    let albedo_texture : vec4f = textureSample(albedo_texture, sampler_2d, albedo_uv);
    albedo_color = albedo_texture.rgb * in.color.rgb;
    alpha = albedo_texture.a * in.color.a;
#else
    albedo_color = in.color.rgb;
    alpha = in.color.a;
#endif // ALBEDO_TEXTURE

#ifdef ALPHA_MASK
    if (alpha < alpha_cutoff) {
        discard;
    }
#endif

#ifndef UNLIT_MATERIAL

    var m : PbrMaterial;
    m.pos = in.world_position;
    m.view_dir = normalize(camera_data.eye - m.pos);

    get_normal_info(&m, in, is_front_facing);

    m.reflected_dir = normalize(reflect(-m.view_dir, m.normal));
    m.albedo = albedo_color;
    m.f90 = vec3f(1.0);
    m.ior = 1.5; // default IOR for most materials
    m.specular_weight = 1.0;

#ifdef METALLIC_ROUGHNESS_TEXTURE
    let metallic_roughness_uv : vec2f = get_metallic_roughness_uv(in.uv);
    var metal_rough : vec3f = textureSample(metallic_roughness_texture, sampler_2d, metallic_roughness_uv).rgb;
    m.roughness = metal_rough.g * occlusion_roughness_metallic.g;
    m.metallic = metal_rough.b * occlusion_roughness_metallic.b;
#else
    m.roughness = occlusion_roughness_metallic.g;
    m.metallic = occlusion_roughness_metallic.b;
#endif

#ifdef EMISSIVE_TEXTURE
    let emissive_uv : vec2f = get_emissive_uv(in.uv);
    m.emissive = textureSample(emissive_texture, sampler_2d, emissive_uv).rgb * emissive;
#else
    m.emissive = emissive;
#endif

#ifdef OCLUSSION_TEXTURE
    let occlusion_uv : vec2f = get_occlusion_uv(in.uv);
    m.ao = textureSample(oclussion_texture, sampler_2d, occlusion_uv).r;
    m.ao = m.ao * occlusion_roughness_metallic.r;
#else
    m.ao = 1.0;
#endif // OCLUSSION_TEXTURE

    m.roughness = max(m.roughness, 0.04);
    m.diffuse = mix(m.albedo, vec3f(0.0), m.metallic);
    m.f0 = mix(vec3f(0.04), m.albedo, m.metallic);
    m.f0_dielectric = vec3f(0.04);
    m.clearcoat_fresnel = vec3f(0.0);

#ifdef IRIDESCENCE_MATERIAL
    get_iridescence_info(&m, in);
#endif
#ifdef CLEARCOAT_MATERIAL
    get_clearcoat_info(&m, in);
#endif
#ifdef ANISOTROPY_MATERIAL
    get_anisotropy_info(&m, in);
#endif
#ifdef TRANSMISSION_MATERIAL
    get_transmission_info(&m, in);
#endif

    final_color = get_indirect_light(&m);
    final_color += get_direct_light(&m);
    final_color += m.emissive * (1.0 - m.clearcoat_factor * m.clearcoat_fresnel);
#else
    final_color += albedo_color;
#endif // UNLIT_MATERIAL

    final_color *= camera_data.exposure;
    final_color = tonemap_khronos_pbr_neutral(final_color);

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3(1.0 / 2.2));
    }

    out.color = vec4f(final_color, alpha);

    return out;
}