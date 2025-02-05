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

    if(ui_data.clip_range < 0.0 && in.uv.y < abs(ui_data.clip_range) ) {
        discard;
    }
    else if(ui_data.clip_range > 0.0 && in.uv.y > ui_data.clip_range ) {
        discard;
    }

    let combo_index = ui_data.data_value;
    let is_disabled : bool = (ui_data.flags & UI_DATA_DISABLED) == UI_DATA_DISABLED;
    let is_selected : bool = (ui_data.flags & UI_DATA_SELECTED) == UI_DATA_SELECTED;
    let is_pressed : bool = (ui_data.flags & UI_DATA_PRESSED) == UI_DATA_PRESSED;
    let is_color_button : bool = (ui_data.flags & UI_DATA_COLOR_BUTTON) == UI_DATA_COLOR_BUTTON;
    let hover_transition : f32 = pow(ui_data.hover_time, 3.0);

    var out: FragmentOutput;

#ifdef ALBEDO_TEXTURE
    let corrected_uv : vec2f = vec2f(in.uv.x * ui_data.aspect_ratio - (ui_data.aspect_ratio - 1.0) * 0.5, in.uv.y);
    var color : vec4f = textureSample(albedo_texture, texture_sampler, corrected_uv);
#else
    var color : vec4f = vec4f(in.color.rgb, 1.0);
#endif

    let bra : f32 = mix(0.98, 0.6, hover_transition);
                         // tr   br   tl   bl
    var ra : vec4f = vec4f(bra);
    var si : vec2f = vec2f(0.98 * ui_data.aspect_ratio, 0.98);
    ra = min(ra, min(vec4f(si.x), vec4f(si.y)));

    if(combo_index == 1.0) {
        ra.x = 0.0;
        ra.y = 0.0;
    }
    else if(combo_index == 2.0) {
        ra = vec4f(0.0);
    }
    else if(combo_index == 3.0) {
        ra.z = 0.0;
        ra.w = 0.0;
    }

    if(is_color_button) {
        ra = vec4f(si.x);
    }

    var uvs = vec2f(in.uv.x, 1.0 - in.uv.y);
    var pos : vec2f = vec2(uvs * 2.0 - 1.0);
    pos.x *= ui_data.aspect_ratio;

    let d : f32 = sdRoundedBox(pos, si, ra);

    var final_color : vec3f = color.rgb;

#ifdef ALBEDO_TEXTURE
        let mask : f32 = color.a;
#else
        let mask : f32 = select(0.0, 1.0, is_color_button);
#endif

    if(!is_disabled) {
        let no_hover_color : vec3f = COLOR_DARK;
        var hover_color : vec3f = mix(COLOR_TERCIARY, mix( COLOR_HIGHLIGHT_LIGHT, COLOR_TERCIARY, in.uv.x * in.uv.y ), in.uv.x );

        if(is_pressed) {
            hover_color += vec3f(0.1);
        }

        final_color = mix( mix(no_hover_color, hover_color, hover_transition), final_color, mask );

        if(is_selected) {
            final_color = mix( mix( COLOR_HIGHLIGHT_LIGHT, COLOR_TERCIARY, in.uv.x * in.uv.y ), final_color, mask );
        }

    } else {
        final_color = mix( COLOR_SECONDARY, final_color, mask ) * 0.3;
    }

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }

    final_color = mix( final_color, vec3f(0.3), 1.0 - smoothstep(0.08, 0.16, abs(d)) );

    var alpha : f32 = 1.0 - smoothstep(0.0, 0.04, d);

    out.color = vec4f(final_color, alpha);

    return out;
}