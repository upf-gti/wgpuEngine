
struct VertexInput {
    @builtin(instance_index) instance_id : u32,
#vertex buffer(0) @location(0) position: vec3f,
#ifdef UV_0
#vertex buffer(1) @location(1) uv0: vec2f,
#endif
#ifdef UV_1
#vertex buffer(2) @location(2) uv1: vec2f,
#endif
#vertex buffer(3) @location(3) normal: vec3f,
#vertex buffer(3) @location(4) tangent: vec4f,
#vertex buffer(3) @location(5) color: vec3f,
#vertex buffer(3) @location(6) weights: vec4f,
#vertex buffer(3) @location(7) joints: vec4i
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv0: vec2f,
    @location(1) uv1: vec2f,
    @location(2) color: vec4f,
    @location(3) world_position: vec3f,
    @location(4) normal: vec3f,
    @location(5) tangent : vec3f,
    @location(6) bitangent : vec3f,
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