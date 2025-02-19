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

// from @lingel at https://www.shadertoy.com/view/3tj3R1

const OUTLINEWIDTH : f32 = 0.08;

fn rgb_to_hsv( rgb : vec3f ) -> vec3f
{
    let h_scale : f32 = 60.0;
    let Cmax : f32 = max(max(rgb.x,rgb.y),rgb.z);
    let Cmin : f32 = min(min(rgb.x,rgb.y),rgb.z);
    let delta : f32 = Cmax - Cmin;

    var H : f32 = 0.0;
    var S : f32 = 0.0;

    if(delta != 0.0)
    {
        if(Cmax == rgb.r) {
           H = (rgb.g - rgb.b) / delta;
        } else if (Cmax == rgb.g) {
           H = (rgb.b - rgb.r) / delta + 2.0;
        } else if(Cmax == rgb.b) {
           H = (rgb.r - rgb.g) / delta + 4.0;
        }
    }

    if(Cmax != 0.0)
    {
        S = delta / Cmax;
    }

    H *= h_scale;

    return vec3f(H, S, Cmax);
}

fn hsv_to_rgb( hsv : vec3f ) -> vec3f
{
    var c : vec3f = hsv;
    c.x /= 360.0;
    var K : vec4f = vec4f(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    var p : vec3f = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx,vec3f(0.0),vec3f(1.0)), c.y);
}

fn get_ring_color( uvs : vec2f ) -> vec3f
{
    let xiangxian : f32 = floor(1.0 - uvs.y );
    let radian : f32 = xiangxian * 2.0 * PI + atan2(uvs.y,uvs.x);
    let degree : f32 = radian/(2.0*PI)*360.0;
    let hsv = vec3(degree,1.0,1.0);
    return hsv_to_rgb(hsv);
}

fn hsv_to_position( hsv : vec3f, v1 : vec2f, v2 : vec2f, v3 : vec2f ) -> vec2f
{
    let base_s : vec2f = v1 - v3;
    let base_v : vec2f = base_s * 0.5 - (v2 - v3);
    let base_o : vec2f = v3 - base_v;
    var S : f32 = hsv.g;
    let V = hsv.b;
    S -= 0.5;
    S *= V;
    S += 0.5;
    return base_o + base_s * S + base_v * V;
}

fn position_to_sv( p : vec2f, v1 : vec2f, v2 : vec2f, v3 : vec2f ) -> vec2f
{
    var base_s : vec2f = v1 - v3;
    var base_v : vec2f = base_s * 0.5 - (v2 - v3);
    var base_o : vec2f = v3 - base_v;
    var pp : vec2f = p - base_o;
    var s : f32 = dot(pp, base_s) / pow(length(base_s), 2.0);
    var v : f32 = dot(pp, base_v) / pow(length(base_v), 2.0);
    s -= 0.5;
    s /= v;
    s += 0.5;
    return clamp(vec2f(s, v), vec2f(0.0), vec2f(1.0));
}

fn draw_triangle( uv : vec2f, v1 : vec2f, v2 : vec2f, v3 : vec2f, color : vec3f ) -> vec4f
{
    var triangle_color : vec3f = color;

    // Triangle alpha

    let v1_offset : vec2f = v1 * 1.125;
    let v2_offset : vec2f = v2 * 1.125;
    let v3_offset : vec2f = v3 * 1.125;

    var weight : vec3f = calculate_triangle_weight(uv, v1_offset, v2_offset, v3_offset);
    var alpha : f32 = min(min(weight.x, weight.y), weight.z);
    alpha = smoothstep(-EPSILON * 0.5, EPSILON * 0.5, alpha);

    // Draw outlines

    let line1 : vec4f = draw_line(uv, v1_offset, v2_offset, OUTLINECOLOR, OUTLINEWIDTH);
    let line2 : vec4f = draw_line(uv, v2_offset, v3_offset, OUTLINECOLOR, OUTLINEWIDTH);
    let line3 : vec4f = draw_line(uv, v1_offset, v3_offset, OUTLINECOLOR, OUTLINEWIDTH);

    triangle_color = mix(triangle_color, line1.rgb, line1.a);
    triangle_color = mix(triangle_color, line2.rgb, line2.a);
    triangle_color = mix(triangle_color, line3.rgb, line3.a);

    return vec4f(triangle_color, alpha);
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {

    var dummy = camera_data.eye;

    if(ui_data.clip_range < 0.0 && in.uv.y < abs(ui_data.clip_range) ) {
        discard;
    }
    else if(ui_data.clip_range > 0.0 && in.uv.y > ui_data.clip_range ) {
        discard;
    }

    // Mask button shape
    var dist : f32 = distance(in.uv, vec2f(0.5));
    var button_radius : f32 = 0.475;

    var out: FragmentOutput;

    var uvs : vec2f = in.uv * 2.0 - 1.0;
    uvs.y *= -1.0;

    var current_color = ui_data.data_vec;
    let degree = current_color.r;
    var final_color : vec3f = hsv_to_rgb(current_color);

    let exterior_radius : f32 = 1.0;
    let thickness : f32 = 0.25;
    let interior_radius : f32 = exterior_radius - thickness;

    // Ring alpha
    let position_radius : f32 = length(uvs);
    let interior_mask : f32 = smoothstep(interior_radius, interior_radius + EPSILON, position_radius);
    let outerior_mask : f32 = smoothstep(exterior_radius, exterior_radius - EPSILON, position_radius);
    var alpha = min(interior_mask, outerior_mask);  

    // Add ring
    final_color = mix(final_color, get_ring_color(uvs), alpha);

    // Compute points
    let DEG2RAD : f32 = PI / 180.0;
    var angle : f32 = degree * DEG2RAD; // Use 90.0 in "degree" to skip rotation of the triangle
    let v1 : vec2f = vec2f(cos(angle), sin(angle)) * (interior_radius - OUTLINEWIDTH);
    angle += 120.0 * DEG2RAD;
    let v2 : vec2f = vec2f(cos(angle), sin(angle)) * (interior_radius - OUTLINEWIDTH);
    angle += 120.0 * DEG2RAD;
    let v3 : vec2f = vec2f(cos(angle), sin(angle)) * (interior_radius - OUTLINEWIDTH);

    // Add triangle
    let triangle_color : vec4f = draw_triangle(uvs, v1, v2, v3, hsv_to_rgb(vec3f(degree, position_to_sv(uvs, v1, v2, v3))));
    final_color = mix(final_color, triangle_color.rgb, triangle_color.a);

    // Add HUE line marker
    let v1_offset = v1 * 1.125;
    let line_color : vec4f = draw_line(uvs, v1_offset + normalize(v1_offset) * EPSILON * 0.5, v1_offset + normalize(v1_offset) * (thickness - EPSILON * 0.5), OUTLINECOLOR, 0.025);
    final_color = mix(final_color, line_color.rgb, line_color.a);

    // Add HS point marker
    let point_color : vec4f = draw_point(uvs, hsv_to_position(current_color, v1, v2, v3), 0.025);
    final_color = mix(final_color, point_color.rgb, point_color.a);

    // outer ring line
    final_color = mix(final_color, OUTLINECOLOR.rgb, smoothstep(button_radius - EPSILON, button_radius, dist));

    // inner ring line
    let tmp_interior_mask : f32 = smoothstep(interior_radius - 0.06, interior_radius - 0.06 + EPSILON, position_radius);
    var inner_outline_mask : f32 = (1.0 - interior_mask) - (1.0 - tmp_interior_mask);
    final_color = mix(final_color, OUTLINECOLOR.rgb, inner_outline_mask);

    final_color = pow(final_color, vec3f(2.2));

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }
    
    var shadow : f32 = clamp(1.0 - smoothstep(0.49, 0.5, dist), 0.0, 1.0);

    out.color = vec4f(final_color, shadow);

    return out;
}