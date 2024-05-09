
struct VertexInput {
    @builtin(instance_index) instance_id : u32,
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
    @location(2) normal: vec3f,
    @location(3) tangent: vec3f,
    @location(4) color: vec3f,
    @location(5) weights: vec4f,
    @location(6) joints: vec4i
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) normal: vec3f,
    @location(2) color: vec4f,
    @location(3) world_position: vec3f,
};

struct RenderMeshData {
    model  : mat4x4f
};

struct InstanceData {
    data : array<RenderMeshData>
}

struct CameraData {
    view_projection : mat4x4f,
    eye : vec3f,
    dummy : f32,
    right_controller_position : vec3f,
    dummy2 : f32
};
