#include mesh_includes.wgsl

struct ShadowVertexInput {
    @builtin(instance_index) instance_id : u32,
    @location(0) position: vec3f,
#ifdef USE_SKINNING
#unique vertex @location(5) weights: vec4f,
#unique vertex @location(6) joints: vec4i
#endif
};

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;
@group(1) @binding(0) var<uniform> light_view_projection : mat4x4f;

@vertex
fn vs_main(in : ShadowVertexInput) -> @builtin(position) {

    var position = vec4f(position, 1.0);

#ifdef USE_SKINNING
    var skin : mat4x4f = (animated_matrices[in.joints.x] * inv_bind_matrices[in.joints.x]) * in.weights.x;
    skin += (animated_matrices[in.joints.y] * inv_bind_matrices[in.joints.y]) * in.weights.y;
    skin += (animated_matrices[in.joints.z] * inv_bind_matrices[in.joints.z]) * in.weights.z;
    skin += (animated_matrices[in.joints.w] * inv_bind_matrices[in.joints.w]) * in.weights.w;
    position = skin * position;
    normals = skin * normals;
#endif

    let instance_data : RenderMeshData = mesh_data.data[in.instance_id];

    return scene.lightViewProjMatrix * instance_data.model * vec4(position, 1.0);
}
