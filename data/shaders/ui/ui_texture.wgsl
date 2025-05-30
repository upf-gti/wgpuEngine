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
    out.uv = in.uv;
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
    let dummy1 = ui_data.data_value;

    if(ui_data.clip_range < 0.0 && in.uv.y < abs(ui_data.clip_range) ) {
        discard;
    }
    else if(ui_data.clip_range > 0.0 && in.uv.y > ui_data.clip_range ) {
        discard;
    }

    var out: FragmentOutput;
    var color : vec4f = textureSample(albedo_texture, texture_sampler, in.uv);

    var _color = color.rgb;

    if (GAMMA_CORRECTION == 1) {
        _color = pow(_color, vec3f(1.0 / 2.2));
    }

    out.color = vec4f(_color, color.a);

    return out;
}