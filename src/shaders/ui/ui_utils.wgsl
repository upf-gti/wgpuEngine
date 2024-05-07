const COLOR_PRIMARY         = pow(vec3f(0.976, 0.976, 0.976), vec3f(2.2));
const COLOR_SECONDARY       = pow(vec3f(0.931, 0.912, 0.88), vec3f(2.2));
const COLOR_TERCIARY        = pow(vec3f(1.0, 0.404, 0.0), vec3f(2.2));
const COLOR_HIGHLIGHT_LIGHT = pow(vec3f(0.467, 0.333, 0.933), vec3f(2.2));
const COLOR_HIGHLIGHT       = pow(vec3f(0.26, 0.2, 0.533), vec3f(2.2));
const COLOR_HIGHLIGHT_DARK  = pow(vec3f(0.082, 0.086, 0.196), vec3f(2.2));

const EPSILON : f32 = 0.02;
const PI : f32 = 3.14159265359;

const OUTLINECOLOR : vec4f = vec4f(0.3, 0.3, 0.3, 0.9);

fn calculate_triangle_weight( p : vec2f, v1 : vec2f, v2 : vec2f, v3 : vec2f ) -> vec3f
{
    var weight : vec3f;
    weight.x = ((v2.y-v3.y)*(p.x-v3.x)+(v3.x-v2.x)*(p.y-v3.y)) / ((v2.y-v3.y)*(v1.x-v3.x)+(v3.x-v2.x)*(v1.y-v3.y));
    weight.y = ((v3.y-v1.y)*(p.x-v3.x)+(v1.x-v3.x)*(p.y-v3.y)) / ((v2.y-v3.y)*(v1.x-v3.x)+(v3.x-v2.x)*(v1.y-v3.y));
    weight.z = 1.0 - weight.x - weight.y;
    return weight;
}

fn draw_line( uv : vec2f, p1 : vec2f, p2 : vec2f, color : vec4f, thickness : f32 ) -> vec4f
{
    var final_color : vec4f = color;
    let t = thickness * 0.5;
    var dir = p2 - p1;
    let line_length : f32 = length(dir);
    dir = normalize(dir);
    let to_uv : vec2f = uv - p1;
    let project_length : f32 = dot(dir, to_uv);
    var p2_line_distance : f32 = length(to_uv-dir*project_length);
    p2_line_distance = smoothstep(t + EPSILON, t, p2_line_distance);
    var p2_end_distance : f32 = select(project_length-line_length, abs(project_length), project_length <= 0.0);
    p2_end_distance = smoothstep(t, t - EPSILON * 0.5, p2_end_distance);
    final_color = vec4f(final_color.xyz * mix(p2_line_distance, 0.0, 0.01), final_color.a);
    final_color.a *= min(p2_line_distance, p2_end_distance);
    return final_color;
}

fn draw_point( uv : vec2f, p : vec2f, s : f32) -> vec4f
{
    let alpha : f32 = smoothstep(0.015,0.002, abs(length(uv - p) - s));
    return vec4(vec3(1.0), alpha);
}