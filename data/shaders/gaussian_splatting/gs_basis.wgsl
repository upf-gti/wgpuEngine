// given centroids, covariance, camera_data.view, camera_data.projection;
// compute 2d basis, distance

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
@group(0) @binding(1) var<storage, read> centroid :array<vec4<f32>>;
@group(0) @binding(2) var<storage, read> covariance: array<vec3<f32>>;
@group(0) @binding(3) var<storage, read_write> basis: array<vec4<f32>>;
@group(0) @binding(4) var<storage, read_write> distance: array<u32>;
@group(0) @binding(5) var<storage, read_write> id: array<u32>;
@group(0) @binding(6) var<uniform> splat_count: u32;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

@compute @workgroup_size(256)
fn compute(@builtin(global_invocation_id) GlobalInvocationID : vec3<u32>,
    @builtin(local_invocation_id) LocalInvocationID: vec3<u32>,
    @builtin(workgroup_id) WorkgroupID: vec3<u32>) 
{
    var gid = GlobalInvocationID.x;

    if (gid < splat_count) {
        var covarianceFirst = covariance[gid*2];
        var covarianceSecond = covariance[gid*2+1];

        var covarianceMatrix: mat3x3<f32> = mat3x3(
            covarianceFirst.x,
            covarianceFirst.y,
            covarianceFirst.z,
            covarianceFirst.y,
            covarianceSecond.x,
            covarianceSecond.y,
            covarianceFirst.z,
            covarianceSecond.y,
            covarianceSecond.z);

        var model_view : mat4x4<f32> = camera_data.view * model;

        var model_view_3x3 : mat3x3<f32> = transpose(mat3x3<f32>(model_view[0].xyz, model_view[1].xyz, model_view[2].xyz));

        var cameraFocalLengthX : f32 = camera_data.projection[0][0] *
        camera_data.screen_size.x * 0.5;
        var cameraFocalLengthY : f32 = camera_data.projection[1][1] *
        camera_data.screen_size.y * 0.5;

        var center:vec4<f32> = vec4(centroid[gid].xyz, 1.0);

        var viewCenter:vec4<f32> = model_view * center;
        var s : f32 = 1.0 / (viewCenter.z * viewCenter.z);

        var J = mat3x3<f32>(
            cameraFocalLengthX / viewCenter.z, 0.0, -(cameraFocalLengthX * viewCenter.x) * s,
            0.0, cameraFocalLengthY / viewCenter.z, -(cameraFocalLengthY * viewCenter.y) * s,
            0.0, 0.0, 0.0
        );

        var T : mat3x3<f32> =  model_view_3x3  * J ;

        var T_t : mat3x3<f32> = transpose(T);

        var new_c : mat3x3<f32> = T_t * covarianceMatrix * T;
        
        var cov2Dv:vec3<f32> = vec3<f32>(new_c[0][0] , new_c[0][1], new_c[1][1]);
        
        var a:f32 = cov2Dv.x;
        var b:f32 = cov2Dv.y;
        var d:f32 = cov2Dv.z;

        var D:f32 = a * d - b * b;
        var trace:f32 = a + d;
        var traceOver2:f32 = 0.5 * trace;
        var term2:f32 = sqrt(trace * trace / 4.0 - D);
        
        var eigenValue1 : f32 = traceOver2 + term2;
        var eigenValue2 : f32 = max(traceOver2 - term2, 0.0);

        const maxSplatSize:f32 = 1024.0;
        var eigenVector1 : vec2<f32> = normalize(vec2<f32>(b, eigenValue1 - a));
        var eigenVector2 : vec2<f32> = vec2<f32>(eigenVector1.y, -eigenVector1.x);

        var basisVector1 : vec2<f32> = eigenVector1 * min(sqrt(eigenValue1)*4, maxSplatSize);
        var basisVector2 : vec2<f32> = eigenVector2 * min(sqrt(eigenValue2)*4, maxSplatSize);
        var dis : vec4<f32> = camera_data.projection * model_view * center;
        distance[gid] = u32(dis.z / dis.w * f32(0xFFFFFFFF >> 8));     
        basis[gid] = vec4<f32>(basisVector1, basisVector2);
    }
    else {
        distance[gid] = 0xFFFFFFFF;
    }

    id[gid] = gid;
}