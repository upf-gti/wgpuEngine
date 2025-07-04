const M_PI = 3.14159265359;
const M_PI_INV = 1.0 / 3.14159265359;

fn sq(x : f32) -> f32
{
    return x * x;
}

fn sq3(v : vec3f) -> vec3f
{
    return v * v;
}

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

// iquilez: https://www.shadertoy.com/view/3s33zj
// Use to transform normals with transformation of arbitrary
// non-uniform scales (including negative) and skewing. The
// code assumes the last column of m is [0,0,0,1].
fn adjoint( m : mat4x4f ) -> mat3x3f
{
    return mat3x3(cross(m[1].xyz, m[2].xyz),
                  cross(m[2].xyz, m[0].xyz),
                  cross(m[0].xyz, m[1].xyz));
}

// http://www.thetenthplanet.de/archives/1180
fn cotangent_frame( normal : vec3f, p : vec3f, uv : vec2f ) -> mat3x3f
{
    let pos_dx : vec3f = dpdx(p);
    let pos_dy : vec3f = dpdy(p);
    let tex_dx : vec3f = dpdx(vec3f(uv, 0.0));
    let tex_dy : vec3f = dpdy(vec3f(uv, 0.0));
    var t : vec3f = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) / (tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);

    t = normalize(t - normal * dot(normal, t));
    let b : vec3f = normalize(cross(normal, t));

    return mat3x3(t, b, normal);
}

fn perturb_normal( N : vec3f, V : vec3f, texcoord : vec2f, normal_color : vec3f ) -> vec3f
{
    // assume N, the interpolated vertex normal and
    // V, the view vector (vertex to eye)
    var TBN : mat3x3f = cotangent_frame(N, -V, texcoord);
    return normalize(TBN * normal_color);
}