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

    let curvature : f32 = 0.25;

    let uv_centered : vec2f = in.uv * vec2f(2.0) - vec2f(1.0);
    let curve_factor : f32 = 1.0 - (abs(uv_centered.x * uv_centered.x) * 0.5 + 0.5);
    var curved_pos : vec3f = in.position;
    curved_pos.z -= curvature * curve_factor;

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
    let dummy1 = ui_data.num_group_items;

    var out: FragmentOutput;

#ifdef ALBEDO_TEXTURE
    // let corrected_uv : vec2f = vec2f(in.uv.x * ui_data.aspect_ratio - (ui_data.aspect_ratio - 1.0) * 0.5, in.uv.y);
    var color : vec4f = textureSample(albedo_texture, texture_sampler, in.uv);
#else
    var color : vec4f = vec4f(in.color.rgb, 1.0);
#endif

    var final_color = color.rgb * color.rgb;

    var ra : vec4f = vec4f(0.15);
    var si : vec2f = vec2f(0.98 * 2.0, 0.98) * ui_data.inner_scale;
    var uvs = vec2f(in.uv.x, 1.0 - in.uv.y) + vec2f(0.2);
    var pos : vec2f = vec2(uvs * 2.0 - 1.0);
    pos.x *= 2.0;

    let d : f32 = sdRoundedBox(pos, si, ra);

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }

    var alpha : f32 = (1.0 - smoothstep(0.0, 0.04, d)) * color.a;

    out.color = vec4f(final_color, alpha);

    return out;
}