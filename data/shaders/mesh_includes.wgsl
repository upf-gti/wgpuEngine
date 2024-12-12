
struct VertexInput {
    @builtin(instance_index) instance_id : u32,
    @location(0) position: vec3f,
#unique vertex @location(1) uv: vec2f,
#unique vertex @location(2) normal: vec3f,
#unique vertex @location(3) tangent: vec4f,
#unique vertex @location(4) color: vec3f,
#unique vertex @location(5) weights: vec4f,
#unique vertex @location(6) joints: vec4i
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) color: vec4f,
    @location(2) world_position: vec3f,
    @location(3) normal: vec3f,
    @location(4) tangent : vec3f,
    @location(5) bitangent : vec3f,
};

struct RenderMeshData {
    model  : mat4x4f
};

struct InstanceData {
    data : array<RenderMeshData>
}

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