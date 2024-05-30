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

                         // tr   br   tl   bl
    var ra : vec4f = vec4f(0.98);
    var si : vec2f = vec2f(0.98 * ui_data.num_group_items, 0.98);
    ra = min(ra, min(vec4f(si.x), vec4f(si.y)));

    var uvs = vec2f(in.uv.x, 1.0 - in.uv.y);
    var pos : vec2f = vec2(uvs * 2.0 - 1.0);
    pos.x *= ui_data.num_group_items;

    let d : f32 = sdRoundedBox(pos, si, ra);
    
    var final_color : vec3f = in.color.rgb;
    
    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }

    final_color = mix( final_color, vec3f(0.3), 1.0 - smoothstep(0.08, 0.1, abs(d)) );

    var alpha : f32 = select(0.0, 1.0, d > 0.0);
    alpha = smoothstep(0.0, 0.04, d);

    out.color = vec4f(final_color, 1.0 - alpha);
    
    return out;
}