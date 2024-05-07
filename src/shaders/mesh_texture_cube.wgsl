#include mesh_includes.wgsl
#include tonemappers.wgsl

#define GAMMA_CORRECTION

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

@group(2) @binding(0) var irradiance_texture: texture_cube<f32>;
@group(2) @binding(1) var<uniform> albedo: vec4f;
@group(2) @binding(7) var sampler_clamp : sampler;

struct SkyboxVertexOutput {
    @builtin(position) position: vec4f,
    @location(0) vertex_position: vec3f,
    @location(1) world_position: vec3f,
    @location(2) color: vec4f
};


@vertex
fn vs_main(in: VertexInput) -> SkyboxVertexOutput {

    let instance_data : RenderMeshData = mesh_data.data[in.instance_id];

    var out: SkyboxVertexOutput;
    var world_position = instance_data.model * vec4f(in.position, 1.0);
    out.world_position = world_position.xyz;
    out.position = camera_data.view_projection * world_position;
    out.vertex_position = in.position;
    out.color = vec4(in.color, 1.0) * albedo;
    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f
}

@fragment
fn fs_main(in: SkyboxVertexOutput) -> FragmentOutput {
    
    var view = normalize(in.world_position - camera_data.eye);

    var out: FragmentOutput;
    var final_color : vec3f = textureSampleLevel(irradiance_texture, sampler_clamp, in.vertex_position, 1.0).rgb;
    
    final_color = tonemap_khronos_pbr_neutral(final_color);

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3(1.0 / 2.2));
    }

    out.color = vec4f(final_color, 1.0);

    return out;
}