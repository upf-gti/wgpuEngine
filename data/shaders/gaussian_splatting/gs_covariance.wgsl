 @group(0) @binding(0) var<storage, read> rotation :array<vec4<f32>>;
 @group(0) @binding(1) var<storage, read> scaling :array<vec4<f32>>;
 @group(0) @binding(2) var<storage, read_write> covariance: array<vec3<f32>>;
 @group(0) @binding(6) var<uniform> splat_count: u32;

fn quat2mat(quat: vec4<f32>) -> mat3x3<f32>
{
    let x2 = quat.x + quat.x;
    let y2 = quat.y + quat.y;
    let z2 = quat.z + quat.z;
    let xx = quat.x * x2;
    let yx = quat.y * x2;
    let yy = quat.y * y2;
    let zx = quat.z * x2;
    let zy = quat.z * y2;
    let zz = quat.z * z2;
    let wx = quat.w * x2;
    let wy = quat.w * y2;
    let wz = quat.w * z2;
    var out:mat3x3<f32>;
    out[0][0] = 1 - yy - zz;
    out[0][1] = yx + wz;
    out[0][2] = zx - wy;

    out[1][0] = yx - wz;
    out[1][1] = 1.0 - xx - zz;
    out[1][2] = zy + wx;

    out[2][0] = zx + wy;
    out[2][1] = zy - wx;
    out[2][2] = 1.0 - xx - yy;

    return out;
}

fn scaling2mat(scaling:vec3<f32>) ->mat3x3<f32>
{
    var out:mat3x3<f32>;
    out[0][0] = scaling.x;
    out[0][1] = 0.0;
    out[0][2] = 0.0;

    out[1][0] = 0.0;
    out[1][1] = scaling.y;
    out[1][2] = 0.0;

    out[2][0] = 0.0;
    out[2][1] = 0.0;
    out[2][2] =  scaling.z;

    return out;
}

@compute @workgroup_size(256)
fn compute(@builtin(global_invocation_id) GlobalInvocationID : vec3<u32>,
    @builtin(local_invocation_id) LocalInvocationID: vec3<u32>,
    @builtin(workgroup_id) WorkgroupID: vec3<u32>) 
{
    if (GlobalInvocationID.x < splat_count) {

        var rotationMatrix: mat3x3<f32> = quat2mat(rotation[GlobalInvocationID.x]);
        var scalingMatrix: mat3x3<f32> = scaling2mat(scaling[GlobalInvocationID.x].xyz);
        var T: mat3x3<f32> = rotationMatrix * scalingMatrix;
        var T_t: mat3x3<f32> = transpose(T);
        
        var covarianceMatrix =  T * T_t;

        covariance[GlobalInvocationID.x * 2] = vec3<f32>(covarianceMatrix[0][0], covarianceMatrix[1][0], covarianceMatrix[2][0]);
        covariance[GlobalInvocationID.x * 2 + 1] = vec3<f32>(covarianceMatrix[1][1], covarianceMatrix[2][1], covarianceMatrix[2][2]);
    }
}