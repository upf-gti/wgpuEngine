#include ui_includes.wgsl
#include ../mesh_includes.wgsl

#define GAMMA_CORRECTION

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

#ifdef ALBEDO_TEXTURE
@group(2) @binding(0) var albedo_texture: texture_2d<f32>;
#endif

@group(2) @binding(1) var<uniform> albedo: vec4f;

#ifdef USE_SAMPLER
@group(2) @binding(7) var texture_sampler : sampler;
#endif

@group(3) @binding(0) var<uniform> ui_data : UIData;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {

    let instance_data : RenderMeshData = mesh_data.data[in.instance_id];
    var out: VertexOutput;

    let curvature : f32 = 0.15;

    let uv_centered : vec2f = in.uv * vec2f(2.0) - vec2f(1.0);
    let curve_factor : f32 = 1.0 - (abs(uv_centered.x * uv_centered.x) * 0.5 + 0.5);
    var curved_pos : vec3f = in.position;
    // curved_pos.z -= curvature * curve_factor;

    var world_position : vec4f = instance_data.model * vec4f(curved_pos, 1.0);
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

    let is_hovered : bool = ui_data.hover_info.x > 0.0;
    let is_pressed : bool = ui_data.press_info.x > 0.0;

    var out: FragmentOutput;

    let global_scale : f32 = 0.95;
    let size : vec2f = ui_data.xr_info.xy;
    let is_button : bool = size.x != 1.0;
    
    var position : vec2f = ui_data.xr_info.zw;

    var corrected_uv : vec2f = vec2f(in.uv.x, in.uv.y);
    corrected_uv = corrected_uv / size;
    corrected_uv = corrected_uv - (position / size) + 0.5;
    corrected_uv.y = 1.0 - corrected_uv.y;

#ifdef ALBEDO_TEXTURE
    var color : vec4f = textureSample(albedo_texture, texture_sampler, corrected_uv);
#else
    var color : vec4f = in.color;
#endif

    var final_color = select(color.rgb, COLOR_SECONDARY, is_button);

    // center stuff
    position -= vec2f(0.5);
    
    var ra : vec4f = vec4f(0.125);
    var si : vec2f = vec2f(ui_data.aspect_ratio, 1.0) * size * global_scale;
    ra = min(ra, min(vec4f(si.x), vec4f(si.y)));
    var uvs = vec2f(in.uv.x, in.uv.y) - position;
    var pos : vec2f = vec2(uvs * 2.0 - 1.0);
    pos.x *= ui_data.aspect_ratio;

    let d : f32 = sdRoundedBox(pos, si, ra);

    if(is_hovered) {
        final_color = mix(COLOR_TERCIARY, COLOR_HIGHLIGHT_LIGHT, pow(corrected_uv.y, 2.0));
    }

    if(is_pressed) {
        final_color = COLOR_PRIMARY;
    }

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }

    final_color = mix( final_color, vec3f(0.3), 1.0 - smoothstep(0.0, 0.01, abs(d)) );

    var alpha : f32 = (1.0 - smoothstep(0.0, 0.02, d)) * color.a;

    if(is_button) {
        alpha += (1.0 - smoothstep(0.0, 0.015, abs(d)));
        final_color = mix( final_color, vec3f(0.5), 1.0 - smoothstep(0.0, 0.015, abs(d)) );
    }

    out.color = vec4f(final_color, alpha);

    return out;
}