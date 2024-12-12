
struct CameraData {
    view_projection : mat4x4f,
    view : mat4x4f,
    projection : mat4x4f,
    eye : vec3f,
    exposure : f32,
    right_controller_position : vec3f,
    ibl_intensity : f32,
    screen_size : vec2f,
    dummy : vec2f,
};

@group(0) @binding(0) var<uniform> model : mat4x4f;
@group(0) @binding(1) var<storage, read> centroid : array<vec4<f32>>;
@group(0) @binding(2) var<storage, read> color : array<vec4<f32>>;
@group(0) @binding(3) var<storage, read> basis : array<vec4<f32>>;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

struct VertexInput {
    @location(0) position: vec2<f32>,
#unique instance @location(1) id: u32
};

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) coord: vec2<f32>,
    @location(1) color: vec4<f32>
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput 
{
    var out: VertexOutput;

    var clip_center : vec4<f32> = camera_data.projection * camera_data.view * model * vec4<f32>(centroid[in.id].xyz, 1.0);
    var ndc_center : vec3<f32> = clip_center.xyz / clip_center.w;
    var basis_viewport : vec2<f32> = vec2<f32>(2.0 / camera_data.screen_size.x, 2.0 / camera_data.screen_size.y);
    var ndc_offset : vec2<f32> = vec2(in.position.x * basis[in.id].xy + in.position.y * basis[in.id].zw) * basis_viewport;

    out.coord = in.position;
    out.color = color[in.id];

    out.clip_position = vec4<f32>(ndc_center.xy + ndc_offset, ndc_center.z, 1.0);

    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> 
{
    //var a = 1.0/(0.25 * sqrt(2.0*3.14))*exp(-0.5 * (in.coord.x*in.coord.x+in.coord.y*in.coord.y)/(0.25*0.25));
    var vPosition = in.coord * 2.0;

    var A = -dot(vPosition, vPosition);
    if (A < -4.0) {
        discard;
    }

    A = exp(A) * in.color.a;

    //  gl_FragColor = vec4(color.rgb, A);
    var a = 1.0 / (0.25 * sqrt(2.0*3.14)) * exp(-0.5 * (in.coord.x*in.coord.x+in.coord.y*in.coord.y)/(0.25*0.25));
    return vec4<f32>(in.color.rgb * camera_data.exposure, a * in.color.a);
}