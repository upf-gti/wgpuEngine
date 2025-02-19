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
#ifdef UV_0
    out.uv0 = in.uv0;
#endif
    out.color = vec4(vec3f(abs(in.position.z)), 1.0) * albedo;
    out.normal = in.normal;
    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f
}

const COLOR_PRIMARY         = pow(vec3f(0.976, 0.976, 0.976), vec3f(2.2));
const COLOR_SECONDARY       = pow(vec3f(0.967, 0.892, 0.793), vec3f(2.2));
const COLOR_TERCIARY        = pow(vec3f(1.0, 0.404, 0.0), vec3f(2.2));
const COLOR_HIGHLIGHT_LIGHT = pow(vec3f(0.467, 0.333, 0.933), vec3f(2.2));
const COLOR_HIGHLIGHT       = pow(vec3f(0.26, 0.2, 0.533), vec3f(2.2));
const COLOR_HIGHLIGHT_DARK  = pow(vec3f(0.082, 0.086, 0.196), vec3f(2.2));
const COLOR_DARK            = pow(vec3f(0.172, 0.172, 0.172), vec3f(2.2));

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {

    var out: FragmentOutput;
    var dummy = camera_data.eye;

    var y : f32 = abs(in.color.r * 2.0 - 1.0);
    var f : f32 = 1.0 - clamp(pow(y, 0.5), 0.0, 1.0);

    var color : vec3f = mix( COLOR_TERCIARY, COLOR_HIGHLIGHT_LIGHT * 1.25, in.color.r );

    if (GAMMA_CORRECTION == 1) {
        color = pow(color, vec3f(1.0 / 2.2));
    }

    out.color = vec4f(color, f);

    return out;
}