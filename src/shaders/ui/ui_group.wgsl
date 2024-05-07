#include ui_includes.wgsl
#include ../mesh_includes.wgsl

#define GAMMA_CORRECTION

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

@group(2) @binding(1) var<uniform> albedo: vec4f;

@group(3) @binding(0) var<uniform> ui_data : UIData;

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

    var dummy = camera_data.eye;

    var out: FragmentOutput;

    var uvs = in.uv;
    var tx = max(UI_BUTTON_SIZE, UI_BUTTON_SIZE * ui_data.num_group_items);
    var divisions = tx / UI_BUTTON_SIZE;
    uvs.x *= divisions;
    var p = vec2f(clamp(uvs.x, 0.5, divisions - 0.5), 0.5);
    var dist = distance(uvs, p);
    
    var button_radius : f32 = 0.4;
    let back_color :vec3f = in.color.rgb;

    var shadowFactor : f32 = smoothstep(button_radius, 0.5, dist);
    var final_color : vec3f = back_color;
    
    if(dist > button_radius) {
        final_color *= 0.5;
    }

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }

    out.color = vec4f(final_color, 1.0 - shadowFactor);
    
    return out;
}