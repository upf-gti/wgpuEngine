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

fn remap_range(oldValue : f32, oldMin: f32, oldMax : f32, newMin : f32, newMax : f32) -> f32 {
    return (((oldValue - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {

    var dummy = camera_data.eye;

    // Alpha mask
    var uvs_mask = in.uv;
    var tx = max(UI_BUTTON_SIZE, UI_BUTTON_SIZE * ui_data.num_group_items);
    var divisions = tx / UI_BUTTON_SIZE;
    uvs_mask.x *= divisions;
    uvs_mask.y = 1.0 - uvs_mask.y;
    var sd = vec2f(clamp(uvs_mask.x, 0.5, divisions - 0.5), 0.5);
    var dist = distance(uvs_mask, sd);
    var button_radius : f32 = 0.42;
    
#ifdef ALBEDO_TEXTURE
    var tex_color : vec4f = textureSample(albedo_texture, texture_sampler, in.uv);
#else
    var tex_color : vec4f = vec4f(0.0);
#endif

    var out: FragmentOutput;

    let value = ui_data.slider_value;
    let max_value = ui_data.slider_max;
    let min_value = ui_data.slider_min;

    // add gradient at the end to simulate the slider thumb
    var axis = select( in.uv.x, uvs_mask.y, ui_data.num_group_items == 1.0 );

    var mesh_color = mix( COLOR_HIGHLIGHT_LIGHT, COLOR_TERCIARY, pow(axis, 1.5));

    if(ui_data.is_hovered > 0.0) {
        mesh_color *= 1.5;
    }

    let percent : f32 = remap_range(value, min_value, max_value, 0.0, 1.0);

    // texture
    let alpha : f32 = 1.0 - smoothstep(0.95, tex_color.a, 0.0);

    var grad = smoothstep(percent - 0.05, percent, axis);
    grad -= smoothstep(percent, percent + 0.05, axis);
    mesh_color += grad;
    mesh_color = mix(mesh_color, tex_color.rgb, alpha);

    let back_color = vec3f(0.02);
    var final_color : vec3f = select( mesh_color, back_color + tex_color.rgb * 0.3 * alpha, axis > percent );
    
    var shadow : f32 = smoothstep(button_radius, 0.5, dist);
    
    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }

    out.color = vec4f(final_color, 1.0 - shadow);

    return out;
}