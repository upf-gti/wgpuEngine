#include mesh_includes.wgsl

#define GAMMA_CORRECTION

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

@group(2) @binding(0) var albedo_texture : texture_3d<f32>;
@group(2) @binding(1) var<uniform> albedo : vec4f;
@group(2) @binding(7) var texture_sampler : sampler;

//@group(2) @binding(12) var<uniform> quality_absorption_scattering: vec3f;

struct VolumesVertexOutput {
    @builtin(position) position: vec4f,
    @location(0) vertex_position: vec3f,
    @location(1) world_position: vec3f,
    @location(2) color: vec4f,
    @location(3) eye_in_local: vec3f
};

// From https://gist.github.com/rsms/9d9e7c4eadf9fe23da0bf0bfb96bc2e6
fn inverse(m :mat4x4f) -> mat4x4f {
        // Note: wgsl does not have an inverse() (matrix inverse) function built in.
        // Source adapted from https://github.com/glslify/glsl-inverse/blob/master/index.glsl
        let a00 = m[0][0];
        let a01 = m[0][1];
        let a02 = m[0][2];
        let a03 = m[0][3];
        let a10 = m[1][0];
        let a11 = m[1][1];
        let a12 = m[1][2];
        let a13 = m[1][3];
        let a20 = m[2][0];
        let a21 = m[2][1];
        let a22 = m[2][2];
        let a23 = m[2][3];
        let a30 = m[3][0];
        let a31 = m[3][1];
        let a32 = m[3][2];
        let a33 = m[3][3];
        let b00 = a00 * a11 - a01 * a10;
        let b01 = a00 * a12 - a02 * a10;
        let b02 = a00 * a13 - a03 * a10;
        let b03 = a01 * a12 - a02 * a11;
        let b04 = a01 * a13 - a03 * a11;
        let b05 = a02 * a13 - a03 * a12;
        let b06 = a20 * a31 - a21 * a30;
        let b07 = a20 * a32 - a22 * a30;
        let b08 = a20 * a33 - a23 * a30;
        let b09 = a21 * a32 - a22 * a31;
        let b10 = a21 * a33 - a23 * a31;
        let b11 = a22 * a33 - a23 * a32;
        let det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
        return mat4x4f(
          vec4<f32>(
            a11 * b11 - a12 * b10 + a13 * b09,
            a02 * b10 - a01 * b11 - a03 * b09,
            a31 * b05 - a32 * b04 + a33 * b03,
            a22 * b04 - a21 * b05 - a23 * b03),
          vec4<f32>(
            a12 * b08 - a10 * b11 - a13 * b07,
            a00 * b11 - a02 * b08 + a03 * b07,
            a32 * b02 - a30 * b05 - a33 * b01,
            a20 * b05 - a22 * b02 + a23 * b01),
          vec4<f32>(
            a10 * b10 - a11 * b08 + a13 * b06,
            a01 * b08 - a00 * b10 - a03 * b06,
            a30 * b04 - a31 * b02 + a33 * b00,
            a21 * b02 - a20 * b04 - a23 * b00),
          vec4<f32>(
            a11 * b07 - a10 * b09 - a12 * b06,
            a00 * b09 - a01 * b07 + a02 * b06,
            a31 * b01 - a30 * b03 - a32 * b00,
            a20 * b03 - a21 * b01 + a22 * b00)
        ) * (1.0 / det);
}

@vertex
fn vs_main(in: VertexInput) -> VolumesVertexOutput {

    let instance_data : RenderMeshData = mesh_data.data[in.instance_id];

    var out: VolumesVertexOutput;
    var world_position = instance_data.model * vec4f(in.position, 1.0);
    out.world_position = world_position.xyz;
    out.position = camera_data.view_projection * world_position;
#ifdef UV_0
    out.uv0 = in.uv0;
#endif
    out.color = vec4(in.color, 1.0) * albedo;
    //out.normal = in.normal;
    out.vertex_position = in.position.xyz;
    
    let inverse_model : mat4x4f = inverse(instance_data.model);
    let eye_in_local_aux : vec4f = inverse_model * vec4f(camera_data.eye, 1.0);
    //out.eye_in_local : vec3f = vec3f(eye_in_local_aux.x / eye_in_local_aux.w, eye_in_local_aux.y / eye_in_local_aux.w, eye_in_local_aux.z / eye_in_local_aux.w);
    out.eye_in_local = eye_in_local_aux.xyz;
    
    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f
}

// Reference: https://gist.github.com/DomNomNom/46bb1ce47f68d255fd5d
fn intersectAABB(rayOrigin : vec3f, rayDir : vec3f, boxMin : vec3f, boxMax : vec3f) -> vec2f
{
    let tMin : vec3f = (boxMin - rayOrigin) / rayDir;
    let tMax : vec3f = (boxMax - rayOrigin) / rayDir;
    let t1 : vec3f = min(tMin, tMax);
    let t2 : vec3f = max(tMin, tMax);
    let tNear : f32 = max(max(t1.x, t1.y), t1.z);
    let tFar : f32 = min(min(t2.x, t2.y), t2.z);
    return vec2f(tNear, tFar);
}

const M_PI : f32 = 3.1415926535897932384626433832795;

fn phase(g : f32, cos_theta : f32) -> f32
{
    let denom : f32 = 1.0 + g * g - 2.0 * g * cos_theta;
    return 1.0 / (4.0 * M_PI) * (1.0 - g * g) / (denom * sqrt(denom));
}

@fragment
fn fs_main(in: VolumesVertexOutput) -> FragmentOutput {
    
    var dummy = camera_data.eye;

    var color = vec3f(0.0);
    var out: FragmentOutput;

    let u_step_length : f32 = 0.001f;
    let ray_dir : vec3f = normalize(in.vertex_position - in.eye_in_local);
    let ray_step : vec3f = ray_dir * u_step_length;
    let ray_origin : vec3f = in.vertex_position;

    var sample_ray : vec3f = ray_origin;
    var sample_ray_light : vec3f = sample_ray;

    let near_far : vec2f = intersectAABB(ray_origin, ray_dir, vec3f(-1.0f), vec3f(1.0f));
    let inner_distance : f32 = near_far.y - near_far.x;
    let num_samples : u32 = u32(inner_distance / u_step_length);

    var transmittance : f32 = 1.0f;
    var illumination : vec3f = vec3f(0.0);

    for (var i : u32 = 0u; i < num_samples; i++) {

        var sample_density : f32 = textureSampleLevel(albedo_texture, texture_sampler, (sample_ray + vec3f(1.0f)) / 2.0f, 0).x;

        let dist : f32 = u_step_length * 1;
        let coef_abso : f32 = 1.0;
        let coef_scat : f32 = 1.0;
        let coef_ext : f32 = (coef_abso + coef_scat);
        transmittance *= exp(-sample_density * coef_ext * dist);
        
        if (sample_density > 0.0)
        {
            // Distance until end of volume
            let ray_dir_light : vec3f = normalize(vec3f(1.5f, 2.0f, 1.5f) - sample_ray);
            let near_far_inside : vec2f = intersectAABB(sample_ray, ray_dir_light, vec3f(-1.0), vec3f(1.0));
            
            // Init variables in raymarching to light
            let u_num_steps_light : i32 = 5;
            let step_length_light : f32 = near_far_inside.y / f32(u_num_steps_light);
            var tau : f32 = 0.0; // also called optical depth   
            sample_ray_light = sample_ray;
            
            // Raymarching for heterogeneous volumes
            for (var j : i32 = 0; j < u_num_steps_light; j++)
            {
                // get sample position
                let sample_ray_light : vec3f = sample_ray + ray_dir_light * step_length_light * f32(j);
                // accumulate density value
                tau += textureSampleLevel(albedo_texture, texture_sampler, (sample_ray_light + vec3(1.0)) / 2.0, 0).x;
            }
            
            let Li_x : f32 = exp(-tau * coef_ext * step_length_light); 

            let cos_theta : f32 = dot(ray_dir, ray_dir_light);
            illumination += Li_x * transmittance * u_step_length * sample_density * phase(0.0, cos_theta) * coef_scat * 10.0;
        }

        sample_ray += ray_step;

        if (transmittance <= 0.0) { break; }
    }

    var final_color : vec3f = illumination;

    if (GAMMA_CORRECTION == 1) {
        final_color = pow(final_color, vec3f(1.0 / 2.2));
    }

    out.color = vec4f(final_color, 1.0 - transmittance);

    return out;
}