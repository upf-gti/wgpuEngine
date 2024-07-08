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

    if(ui_data.range < 0.0 && in.uv.y < abs(ui_data.range) ) {
        discard;
    }
    else if(ui_data.range > 0.0 && in.uv.y > ui_data.range ) {
        discard;
    }

    let hover_transition : f32 = pow(ui_data.hover_info.y, 3.0);
    
    var out: FragmentOutput;

#ifdef ALBEDO_TEXTURE
    var tex_color : vec4f = textureSample(albedo_texture, texture_sampler, in.uv);
#else
    var tex_color : vec4f = vec4f(0.0);
#endif

                         // tr   br   tl   bl
    var ra : vec4f = vec4f(0.98);
    var si : vec2f = vec2f(0.98 * ui_data.aspect_ratio, 0.98);
    ra = min(ra, min(vec4f(si.x), vec4f(si.y)));
    ra = mix(ra, ra * 0.5, hover_transition);

    var uvs = vec2f(in.uv.x, 1.0 - in.uv.y);
    var pos : vec2f = vec2(uvs * 2.0 - 1.0);
    pos.x *= ui_data.aspect_ratio;

    let d : f32 = sdRoundedBox(pos, si, ra);

    let value = ui_data.slider_value;
    let max_value = ui_data.slider_max;
    let min_value = ui_data.slider_min;

    // add gradient at the end to simulate the slider thumb
    var axis = select( uvs.x, uvs.y, ui_data.num_group_items == 1.0 );

    var mesh_color : vec3f = mix( COLOR_HIGHLIGHT_LIGHT, COLOR_TERCIARY, pow(axis, 1.5));
    mesh_color = mix(mesh_color, COLOR_PRIMARY, max(uvs.y - 0.65, 0.0));

    if(ui_data.hover_info.x > 0.0) {
        mesh_color *= 1.5;
    }

    let percent : f32 = remap_range(value, min_value, max_value, 0.035, 1.0);

    var grad = smoothstep(percent - 0.05, percent, axis);
    grad -= smoothstep(percent, percent + 0.05, axis);
    mesh_color += grad;
    mesh_color = mix(mesh_color, tex_color.rgb, tex_color.a);

    var final_color : vec3f = select( mesh_color, COLOR_DARK + tex_color.rgb * 0.3 * tex_color.a, axis > percent );
    
    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }

    final_color = mix( final_color, vec3f(0.3), 1.0 - smoothstep(0.08, 0.16, abs(d)) );

    var alpha : f32 = 1.0 - smoothstep(0.0, 0.04, d);

    out.color = vec4f(final_color, alpha);

    return out;
}