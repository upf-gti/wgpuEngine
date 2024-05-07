#include ui_utils.wgsl
#include ../mesh_includes.wgsl

#define GAMMA_CORRECTION

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

@group(2) @binding(1) var<uniform> albedo: vec4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {

    let instance_data : RenderMeshData = mesh_data.data[in.instance_id];

    var out: VertexOutput;
    var world_position = instance_data.model * vec4f(in.position, 1.0);
    out.world_position = world_position.xyz;
    out.position = camera_data.view_projection * world_position;
    out.uv = in.uv; // forward to the fragment shader
    out.color = vec4(in.color, 1.0) * albedo;
    out.normal = in.normal;
    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {

    var out: FragmentOutput;
    var dummy = camera_data.eye;

    var f : f32 = in.uv.y * 2.0;

    var color : vec3f = mix( COLOR_TERCIARY, COLOR_HIGHLIGHT_LIGHT, f );

    if (GAMMA_CORRECTION == 1) {
        color = pow(color, vec3f(1.0 / 2.2));
    }

    out.color = vec4f(color, 1.0);
    return out;
}