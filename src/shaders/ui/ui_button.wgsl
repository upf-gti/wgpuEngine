#include ui_includes.wgsl
#include ui_utils.wgsl
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
    let combo_index = ui_data.num_group_items;

    // Mask button shape
    var uvs = in.uv;
    var button_radius : f32 = 0.42;
    if(combo_index == 1.0) {
        if(uvs.x > 0.5) {
            uvs.x = 0.5;
        }
    }
    else if(combo_index == 2.0) {
        uvs.x = 0.5;
    }
    else if(combo_index == 3.0) {
        if(uvs.x < 0.5) {
            uvs.x = 0.5;
        }
    }
    var dist : f32 = distance(uvs, vec2f(0.5));
    var max_radius : f32 = 0.5;

    var out: FragmentOutput;

#ifdef ALBEDO_TEXTURE
    var color : vec4f = textureSample(albedo_texture, texture_sampler, in.uv);
#else
    var color : vec4f = vec4f(1.0);
#endif

    var is_disabled : bool = ui_data.is_button_disabled > 0.0;
    var is_color_button : bool = ui_data.is_color_button > 0.0;
    var keep_colors : bool = (ui_data.keep_rgb + ui_data.is_color_button) > 0.0;
    var is_selected : bool = ui_data.is_selected > 0.0;
    var is_hovered : bool = ui_data.is_hovered > 0.0;

    var selected_color : vec3f = COLOR_HIGHLIGHT_DARK;
    var highlight_color : vec3f = COLOR_SECONDARY;
    var back_color : vec3f = vec3f(0.02);
    var icon_color : vec3f = back_color;
    var gradient_factor : f32 = pow(uvs.y, 2.5);

    if(is_disabled) {
        icon_color = vec3f(0.22, 0.12, 0.08);
        back_color = vec3f(0.1);
    }

    // Assign basic color
    var lum = color.r * 0.3 + color.g * 0.59 + color.b * 0.11;
    var final_color = vec3f( 1.0 - smoothstep(0.15, 0.4, lum) );
    final_color = max(final_color, icon_color);

    if(keep_colors) {
        final_color = color.rgb * in.color.rgb;
        final_color = pow(final_color, vec3f(1.0/2.2));
        highlight_color = vec3f(1.0);
    }

    if(is_selected) {
        if( !keep_colors ) {
            var sel_color = mix( COLOR_HIGHLIGHT_LIGHT, COLOR_TERCIARY, uvs.x * uvs.y );
            icon_color = select( sel_color, sel_color + COLOR_TERCIARY * 0.2, is_hovered );
            final_color = smoothstep(vec3f(0.25), vec3f(0.45), color.rgb) * 0.5;
        }
    } 
    // not selected but hovered
    else if(is_hovered && !keep_colors) {
        highlight_color = mix( COLOR_TERCIARY, COLOR_HIGHLIGHT_LIGHT, gradient_factor );
    }
    else if(is_hovered) {
        icon_color = vec3f(0.15);
        final_color += vec3f(0.1);
    }

    final_color = mix(back_color, icon_color + final_color * highlight_color, color.a);

    var shadow : f32 = smoothstep(button_radius, max_radius, dist);
    
    if(is_color_button) {
        final_color = mix(final_color, OUTLINECOLOR.rgb, smoothstep(button_radius - 0.04, button_radius, dist));

        final_color = mix(final_color, OUTLINECOLOR.rgb, (uvs.x * uvs.y) * 0.5);

        final_color = pow(final_color, vec3f(2.2));
        shadow = smoothstep(max_radius - 0.04, max_radius, dist);
    }

    if(dist > button_radius && !is_color_button) {
        final_color = back_color;
    }

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }

    out.color = vec4f(final_color, 1.0 - shadow);

    return out;
}