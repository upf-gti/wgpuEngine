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

fn draw_triangle( uv : vec2f, v1 : vec2f, v2 : vec2f, v3 : vec2f, color : vec3f ) -> vec4f
{
    var triangle_color : vec3f = color;

    // Triangle alpha

    var weight : vec3f = calculate_triangle_weight(uv, v1, v2, v3);
    var alpha : f32 = min(min(weight.x, weight.y), weight.z);
    alpha = smoothstep(-EPSILON * 0.5, EPSILON * 0.5, alpha);

    // Draw outlines

    let line1 : vec4f = draw_line(uv, v1, v2, vec4f(mix(COLOR_HIGHLIGHT_LIGHT, COLOR_TERCIARY, v1.x), 0.9), 0.04);
    let line2 : vec4f = draw_line(uv, v2, v3, vec4f(mix(COLOR_HIGHLIGHT_LIGHT, COLOR_TERCIARY, v2.x), 0.9), 0.04);
    let line3 : vec4f = draw_line(uv, v1, v3, vec4f(mix(COLOR_HIGHLIGHT_LIGHT, COLOR_TERCIARY, v3.x), 0.9), 0.04);

    triangle_color = mix(triangle_color, line1.rgb, line1.a);
    triangle_color = mix(triangle_color, line2.rgb, line2.a);
    triangle_color = mix(triangle_color, line3.rgb, line3.a);

    return vec4f(triangle_color, alpha);
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {

    var dummy = camera_data.eye;

    var out: FragmentOutput;

    var uvs = in.uv;
    var dist : f32 = distance(uvs, vec2f(0.5));
    var button_radius : f32 = 0.39;
    var shadow : f32 = smoothstep(button_radius, 0.42, dist);

    let is_selected : bool = (ui_data.flags & UI_DATA_SELECTED) == UI_DATA_SELECTED;
    let back_color = in.color.rgb;
    let degree = ui_data.data_vec.r;
    var final_color : vec3f = mix(COLOR_TERCIARY, COLOR_HIGHLIGHT_LIGHT, uvs.y);

    let triangle_size : f32 = 0.1;

    let DEG2RAD : f32 = PI / 180.0;
    var angle : f32 = degree * DEG2RAD;
    let v1 : vec2f = vec2f(cos(angle), sin(angle)) * triangle_size;
    angle += 120.0 * DEG2RAD;
    let v2 : vec2f = vec2f(cos(angle), sin(angle)) * triangle_size;
    angle += 120.0 * DEG2RAD;
    let v3 : vec2f = vec2f(cos(angle), sin(angle)) * triangle_size;

    var alpha_offset : f32 = 0.0;

    if(is_selected) {
        var clip_uvs : vec2f = in.uv * 2.0 - 1.0;
        clip_uvs.y *= -1.0;

        let offset : vec2f = normalize(v1) * 0.29;
        let triangle : vec4f = draw_triangle(clip_uvs, v1 + offset, v2 + offset, v3 + offset, COLOR_SECONDARY * 0.8);
        final_color = mix(final_color, triangle.rgb, triangle.a);
        alpha_offset = triangle.a;
    }

    final_color = mix(final_color, vec3f(0.05), smoothstep(button_radius - EPSILON, button_radius, dist));

    if(dist < button_radius) {
        shadow = 1.0 - smoothstep(0.125, 0.3, dist) - alpha_offset;
    }

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }

    out.color = vec4f(final_color, 1.0 - shadow);
    
    return out;
}