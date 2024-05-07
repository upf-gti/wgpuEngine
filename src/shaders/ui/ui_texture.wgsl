#include ui_includes.wgsl
#include ../mesh_includes.wgsl

#define GAMMA_CORRECTION


@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

@group(2) @binding(0) var albedo_texture: texture_2d<f32>;
@group(2) @binding(7) var texture_sampler : sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {

    let instance_data : RenderMeshData = mesh_data.data[in.instance_id];

    var out: VertexOutput;
    var world_position = instance_data.model * vec4f(in.position, 1.0);
    out.world_position = world_position.xyz;
    out.position = camera_data.view_projection * world_position;
    out.uv = in.uv; // forward to the fragment shader
    out.color = in.color * instance_data.color.rgb;
    out.normal = in.normal;
    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {
    
    var dummy = camera_data.eye;

    var out: FragmentOutput;
    var color : vec4f = textureSampleLevel(albedo_texture, texture_sampler, in.uv, 0.0);

    if (color.a < 0.9) {
        discard;
    }

    var _color = color.rgb;

    if (GAMMA_CORRECTION == 1) {
        _color = pow(_color, vec3f(1.0 / 2.2));
    }

    out.color = vec4f(in.color * _color, color.a);

    return out;
}