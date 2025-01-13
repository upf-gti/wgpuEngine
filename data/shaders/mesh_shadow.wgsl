#include mesh_includes.wgsl

struct ShadowVertexInput {
    @builtin(instance_index) instance_id : u32,
    @location(0) position: vec3f,
#ifdef USE_SKINNING
#unique vertex @location(5) weights: vec4f,
#unique vertex @location(6) joints: vec4i
#endif
};

struct ShadowVertexOutput {
  @location(0) shadowPos: vec3f,
  @location(1) fragPos: vec3f,
  @location(2) fragNorm: vec3f,

  @builtin(position) Position: vec4f,
}

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;
#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

#ifdef USE_SKINNING
@group(2) @binding(10) var<storage, read> animated_matrices: array<mat4x4f>;
@group(2) @binding(11) var<storage, read> inv_bind_matrices: array<mat4x4f>;
#endif

@vertex
fn vs_main(in : ShadowVertexInput) -> @builtin(position) vec4f {

    var position = vec4f(in.position, 1.0);

#ifdef USE_SKINNING
    var skin : mat4x4f = (animated_matrices[in.joints.x] * inv_bind_matrices[in.joints.x]) * in.weights.x;
    skin += (animated_matrices[in.joints.y] * inv_bind_matrices[in.joints.y]) * in.weights.y;
    skin += (animated_matrices[in.joints.z] * inv_bind_matrices[in.joints.z]) * in.weights.z;
    skin += (animated_matrices[in.joints.w] * inv_bind_matrices[in.joints.w]) * in.weights.w;
    position = skin * position;
    normals = skin * normals;
#endif

    let instance_data : RenderMeshData = mesh_data.data[in.instance_id];

    return camera_data.view_projection * instance_data.model * position;
}
