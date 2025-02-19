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
#ifdef UV_0
    out.uv0 = in.uv0;
#endif
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

    var uvs : vec2f = vec2f(in.uv.x, in.uv.y);

    if(ui_data.clip_range < 0.0 && uvs.y < abs(ui_data.clip_range) ) {
        discard;
    }
    else if(ui_data.clip_range > 0.0 && uvs.y > ui_data.clip_range ) {
        discard;
    }

    var out: FragmentOutput;

    var ra : vec4f = vec4f(0.6);
    var si : vec2f = vec2f(0.98 * ui_data.aspect_ratio, 0.98);
    ra = min(ra, min(vec4f(si.x), vec4f(si.y)));

    uvs.y = 1.0 - in.uv.y;
    var pos : vec2f = vec2(uvs * 2.0 - 1.0);
    pos.x *= ui_data.aspect_ratio;

    let d : f32 = sdRoundedBox(pos, si * 0.975, ra * 0.975);
    let d_selected : f32 = sdRoundedBox(pos, si, ra);

    var alpha : f32 = 1.0 - smoothstep(0.0, 0.04, d);
    var alpha_selected : f32 = 1.0 - smoothstep(0.0, 0.08, d_selected);

    let is_selected : bool = (ui_data.flags & UI_DATA_SELECTED) == UI_DATA_SELECTED;
    let is_hovered : bool = (ui_data.flags & UI_DATA_HOVERED) == UI_DATA_HOVERED;
    var selected_mask : f32 = select(0.0, alpha_selected - alpha, is_selected);
    selected_mask = mix(0.0, selected_mask, in.uv.x);
    var color_factor : f32 = select(0.01, 0.05, is_hovered);
    color_factor = mix(color_factor, 0.75, selected_mask);

    var final_color = vec3f(color_factor);
    if(length(in.color.rgb) > 0) {
        final_color = mix(final_color, in.color.rgb, 1.0 - smoothstep(0.1, 0.4, (in.uv.x / (1.0 - in.uv.y)) * ui_data.aspect_ratio));
    }

    let highlight_color : vec3f = mix(COLOR_HIGHLIGHT_LIGHT, COLOR_TERCIARY, uvs.y);
    final_color = mix(final_color, highlight_color, selected_mask);

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }


    out.color = vec4f(final_color, alpha_selected);

    return out;
}