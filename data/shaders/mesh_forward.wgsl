#include math.wgsl
#include mesh_includes.wgsl
#include pbr_functions.wgsl
#include tonemappers.wgsl
#include pbr_material.wgsl

#define MAX_LIGHTS

#ifdef IRIDESCENCE_MATERIAL
#include pbr_iridescence.wgsl
#endif

#ifndef UNLIT_MATERIAL
#include pbr_light.wgsl
#endif

#define GAMMA_CORRECTION

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

#ifdef ALBEDO_TEXTURE
@group(2) @binding(0) var albedo_texture: texture_2d<f32>;
#endif

@group(2) @binding(1) var<uniform> albedo: vec4f;

#ifndef UNLIT_MATERIAL

#ifdef METALLIC_ROUGHNESS_TEXTURE
@group(2) @binding(2) var metallic_roughness_texture: texture_2d<f32>;
#endif

@group(2) @binding(3) var<uniform> occlusion_roughness_metallic: vec3f;

#ifdef EMISSIVE_TEXTURE
@group(2) @binding(5) var emissive_texture: texture_2d<f32>;
#endif

@group(2) @binding(6) var<uniform> emissive: vec3f;

#ifdef OCLUSSION_TEXTURE
@group(2) @binding(9) var oclussion_texture: texture_2d<f32>;
#endif

#ifdef CLEARCOAT_MATERIAL
@group(2) @binding(12) var<uniform> clearcoat_data: vec2f;

#ifdef CLEARCOAT_TEXTURE
@group(2) @binding(13) var clearcoat_texture: texture_2d<f32>;
#endif

#ifdef CLEARCOAT_ROUGHNESS_TEXTURE
@group(2) @binding(14) var clearcoat_roughness_texture: texture_2d<f32>;
#endif

#ifdef CLEARCOAT_NORMAL_TEXTURE
@group(2) @binding(15) var clearcoat_normal_texture: texture_2d<f32>;
#endif

#endif // CLEARCOAT_MATERIAL

#ifdef IRIDESCENCE_MATERIAL
@group(2) @binding(16) var<uniform> iridescence_data: vec4f;

#ifdef IRIDESCENCE_TEXTURE
@group(2) @binding(17) var iridescence_texture: texture_2d<f32>;
#endif

#ifdef IRIDESCENCE_THICKNESS_TEXTURE
@group(2) @binding(18) var iridescence_thickness_texture: texture_2d<f32>;
#endif

#endif // IRIDESCENCE_MATERIAL

#endif // UNLIT_MATERIAL

#ifdef NORMAL_TEXTURE
@group(2) @binding(4) var normal_texture: texture_2d<f32>;
@group(2) @binding(19) var<uniform> normal_scale: f32;
#endif

#ifdef USE_SAMPLER
@group(2) @binding(7) var sampler_2d : sampler;
#endif

#ifdef ALPHA_MASK
@group(2) @binding(8) var<uniform> alpha_cutoff: f32;
#endif

#ifdef USE_SKINNING
@group(2) @binding(10) var<storage, read> animated_matrices: array<mat4x4f>;
@group(2) @binding(11) var<storage, read> inv_bind_matrices: array<mat4x4f>;
#endif

#ifndef UNLIT_MATERIAL
@group(3) @binding(0) var irradiance_texture: texture_cube<f32>;
@group(3) @binding(1) var brdf_lut_texture: texture_2d<f32>;
@group(3) @binding(2) var sampler_clamp: sampler;
@group(3) @binding(3) var<uniform> lights : array<Light, MAX_LIGHTS>;
@group(3) @binding(4) var<uniform> num_lights : u32;
#endif

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
    out.world_position = world_position.xyz;
    out.position = camera_data.view_projection * world_position;
    out.uv = in.uv; // forward to the fragment shader
    out.color = vec4f(in.color, 1.0) * albedo;

    // incorrect with scaling/non-uniform scaling
    //out.normal = normalize((instance_data.model * normals).xyz);
    out.normal = normalize(adjoint(instance_data.model) * normals.xyz);

#ifdef HAS_TANGENTS
    out.tangent = normalize((instance_data.model * vec4(in.tangent.xyz, 0.0)).xyz);
    out.bitangent = normalize(cross(out.normal, out.tangent) * in.tangent.w);
#endif

    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f
}

@fragment
fn fs_main(in: VertexOutput, @builtin(front_facing) is_front_facing: bool) -> FragmentOutput {

    var out: FragmentOutput;
    var dummy = camera_data.eye;
    var m : PbrMaterial;

    m.pos = in.world_position;
    m.view_dir = normalize(camera_data.eye - m.pos);
    m.normal_g = normalize(in.normal);

    if (!is_front_facing) {
        m.normal_g = -m.normal_g;
    }

#ifdef NORMAL_TEXTURE
    var normal_color = textureSample(normal_texture, sampler_2d, in.uv).rgb * 2.0 - 1.0;
    normal_color *= vec3f(normal_scale, normal_scale, 1.0);

#ifdef HAS_TANGENTS
    let TBN : mat3x3f = mat3x3f(in.tangent, in.bitangent, m.normal_g);
    m.normal = normalize(TBN * normalize(normal_color));
#else
    m.normal = perturb_normal(m.normal_g, m.view_dir, in.uv, normal_color);
#endif
#else
    m.normal = m.normal_g;
#endif // NORMAL_TEXTURE

    m.n_dot_v = clamp(dot(m.normal, m.view_dir), 0.0, 1.0);
    m.reflected_dir = normalize(reflect(-m.view_dir, m.normal));

    var alpha : f32 = 1.0;

#ifdef ALBEDO_TEXTURE
    let albedo_texture : vec4f = textureSample(albedo_texture, sampler_2d, in.uv);
    m.albedo = albedo_texture.rgb * in.color.rgb;
    alpha = albedo_texture.a * in.color.a;
#else
    m.albedo = in.color.rgb;
    alpha = in.color.a;
#endif // ALBEDO_TEXTURE

#ifdef ALPHA_MASK
    if (alpha < alpha_cutoff) {
        discard;
    }
#endif

    var final_color : vec3f = vec3f(0.0);

#ifndef UNLIT_MATERIAL

    m.f90 = vec3f(1.0);
    m.ior = 1.5; // default IOR for most materials
    m.specular_weight = 1.0;

#ifdef EMISSIVE_TEXTURE
    m.emissive = textureSample(emissive_texture, sampler_2d, in.uv).rgb * emissive;
#else
    m.emissive = emissive;
#endif

#ifdef OCLUSSION_TEXTURE
    m.ao = textureSample(oclussion_texture, sampler_2d, in.uv).r;
    m.ao = m.ao * occlusion_roughness_metallic.r;
#else
    m.ao = 1.0;
#endif // OCLUSSION_TEXTURE

#ifdef METALLIC_ROUGHNESS_TEXTURE
    var metal_rough : vec3f = textureSample(metallic_roughness_texture, sampler_2d, in.uv).rgb;
    m.roughness = metal_rough.g * occlusion_roughness_metallic.g;
    m.metallic = metal_rough.b * occlusion_roughness_metallic.b;
#else
    m.roughness = occlusion_roughness_metallic.g;
    m.metallic = occlusion_roughness_metallic.b;
#endif

    m.roughness = max(m.roughness, 0.04);
    m.diffuse = mix(m.albedo, vec3f(0.0), m.metallic);
    m.f0 = mix(vec3f(0.04), m.albedo, m.metallic);
    m.f0_dielectric = vec3f(0.04);
    m.clearcoat_fresnel = vec3f(0.0);

#ifdef IRIDESCENCE_MATERIAL

    m.iridescence_factor = iridescence_data.x;
    m.iridescence_ior = iridescence_data.y;
    m.iridescence_thickness = iridescence_data.w;

#ifdef IRIDESCENCE_TEXTURE
    let iridescence_uv : vec2f = in.uv;//getIridescenceUV();
    m.iridescence_factor *= textureSample(iridescence_texture, sampler_2d, iridescence_uv).r;
#endif

#ifdef IRIDESCENCE_THICKNESS_TEXTURE
    let iridescence_thickness_uv : vec2f = in.uv;//getIridescenceThicknessUV();
    let thickness_sampled : f32 = textureSample(iridescence_thickness_texture, sampler_2d, iridescence_thickness_uv).g;
    m.iridescence_thickness = mix(iridescence_data.z, iridescence_data.w, thickness_sampled);
#endif

#endif // IRIDESCENCE_MATERIAL

#ifdef CLEARCOAT_MATERIAL

    m.clearcoat_factor = clearcoat_data.x;
    m.clearcoat_roughness = clearcoat_data.y;
    m.clearcoat_f0 = vec3f(pow((m.ior - 1.0) / (m.ior + 1.0), 2.0));
    m.clearcoat_f90 = vec3f(1.0);

#ifdef CLEARCOAT_TEXTURE
    let clearcoat_uv : vec2f = in.uv;//getClearcoatUV();
    let clearcoat_sample : vec4f = textureSample(clearcoat_texture, sampler_2d, clearcoat_uv);
    m.clearcoat_factor *= clearcoat_sample.r;
#endif

#ifdef CLEARCOAT_ROUGHNESS_TEXTURE
    let clearcoat_roughness_uv : vec2f = in.uv;//getClearcoatRoughnessUV();
    let clearcoat_sample_roughness : vec4f = textureSample(clearcoat_roughness_texture, sampler_2d, clearcoat_roughness_uv);
    m.clearcoat_roughness *= clearcoat_sample_roughness.g;
#endif

#ifdef CLEARCOAT_NORMAL_TEXTURE
    let clearcoat_normal_uv : vec2f = in.uv;//getClearcoatNormalUV();
    var clearcoat_normal : vec3f = textureSample(clearcoat_normal_texture, sampler_2d, clearcoat_normal_uv).rgb * 2.0 - vec3(1.0);

#ifdef HAS_TANGENTS
    let cc_TBN : mat3x3f = mat3x3f(in.tangent, in.bitangent, m.normal_g);
    m.clearcoat_normal = normalize(cc_TBN * normalize(clearcoat_normal));
#else
    m.clearcoat_normal = perturb_normal(m.normal_g, m.view_dir, clearcoat_normal_uv, clearcoat_normal);
#endif
#else
    m.clearcoat_normal = m.normal_g;
#endif // CLEARCOAT_NORMAL_TEXTURE

    m.clearcoat_roughness = clamp(m.clearcoat_roughness, 0.04, 1.0);

#endif // CLEARCOAT_MATERIAL

    final_color = get_indirect_light(&m);

    final_color += get_direct_light(&m);

    final_color += m.emissive * (1.0 - m.clearcoat_factor * m.clearcoat_fresnel);
#else
    final_color += m.albedo;
#endif // UNLIT_MATERIAL

    final_color *= camera_data.exposure;
    final_color = tonemap_khronos_pbr_neutral(final_color);

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3(1.0 / 2.2));
    }

    out.color = vec4f(final_color, alpha);
    //out.color = vec4f(pow(vec3f(m.albedo), vec3(1.0 / 2.2)), alpha);
    //out.color = vec4f(vec3f(m.metallic), alpha);
    //out.color = vec4f((m.normal + vec3f(1.0)) * 0.5, alpha);

    return out;
}