const M_PI = 3.14159265359;
const M_PI_INV = 1.0 / 3.14159265359;

fn quat_mult(q1 : vec4f, q2 : vec4f) -> vec4f
{
    return vec4f(
        q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz),
        q1.w * q2.w - dot(q1.xyz, q2.xyz)
    );
}

fn quat_conj(q : vec4f) -> vec4f
{
    return vec4f(-q.x, -q.y, -q.z, q.w);
}

fn quat_inverse(q : vec4f) -> vec4f
{
    let conj : vec4f = quat_conj(q);
    return conj / (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

fn quat_to_mat3(q : vec4f) -> mat3x3f {
    let n = 1.0 / (sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w));
    let qw = q.w * n;
    let qx = q.x * n;
    let qy = q.y * n;
    let qz = q.z * n;

    return mat3x3f(
    1.0 - 2.0*qy*qy - 2.0*qz*qz, 2.0*qx*qy - 2.0*qz*qw, 2.0*qx*qz + 2.0*qy*qw,
    2.0*qx*qy + 2.0*qz*qw, 1.0 - 2.0*qx*qx - 2.0*qz*qz, 2.0*qy*qz - 2.0*qx*qw,
    2.0*qx*qz - 2.0*qy*qw, 2.0*qy*qz + 2.0*qx*qw, 1.0 - 2.0*qx*qx - 2.0*qy*qy);
}


// fn cross_cross(r : vec4f, p : vec3f) -> vec3f {
//     let x : f32 = r.y* r.x*p.y    -    r.y*r.y*p.x    +    r.y*r.w*p.z    +    r.z*r.x*p.z    -    r.z*r.z*p.x    -    r.z*r.w*p.y; 
//     let y : f32 = -r.x* r.x*p.y    +    r.x*r.y*p.x    -    r.x*r.w*p.z    +    r.z*r.y*p.z    -    r.z*r.z*p.y    +    r.z*r.w*p.x; // este ultimo singo no se
//     let z : f32 = -r.x* r.x*p.z    +    r.x*r.z*p.x    +    r.x*r.w*p.y    -    r.y*r.y*p.z    +    r.y*r.z*p.y    -    r.y*r.w*p.x; 

//     return vec3f(x,y,z);
// }

fn rotate_point_quat(position : vec3f, rotation : vec4f) -> vec3f
{
    // let q_pos : vec4f = vec4f(position.x, position.y, position.z, 0.0);
    // let pr_h = quat_mult(quat_mult(rotation, q_pos), quat_conj(rotation));
    // return pr_h.xyz;
    //return position + 2.0 * cross_cross(rotation, position); //cross(rotation.xyz, cross(rotation.xyz, position) + rotation.w * position);
    return position + 2.0 * cross(rotation.xyz, cross(rotation.xyz, position) + rotation.w * position);
}
