
// SD Primitives

const SD_PLANE          = 0;
const SD_SPHERE         = 1;
const SD_BOX            = 2;
const SD_ELLIPSOID      = 3;
const SD_CONE           = 4;
const SD_PYRAMID        = 5;
const SD_CYLINDER       = 6;
const SD_CAPSULE        = 7;
const SD_TORUS          = 8;
const SD_CAPPED_TORUS   = 9;

// SD Operations

const OP_UNION                  = 0;
const OP_SUBSTRACTION           = 1;
const OP_INTERSECTION           = 2;
const OP_SMOOTH_UNION           = 3;
const OP_SMOOTH_SUBSTRACTION    = 4;
const OP_SMOOTH_INTERSECTION    = 5;

// Data containers

struct Surface {
    distance : f32,
    color    : vec3f,
};

struct Edit {
    position   : vec3f,
    radius     : f32,
    size       : vec3f,
    blendOp    : u32,
    color      : vec3f,
    primitive  : u32,
};

// Primitives

fn sdPlane( p : vec3f, c : vec3f, n : vec3f, h : f32, color : vec3f ) -> Surface
{
    // n must be normalized
    var sf : Surface;
    sf.distance = dot(p - c, n) + h;
    sf.color = color;
    return sf;
}

fn sdSphere( p : vec3f, c : vec3f, s : f32, color : vec3f) -> Surface
{
    var sf : Surface;
    sf.distance = length(p - c) - s;
    sf.color = color;
    return sf;
}

// Primitive combinations

fn colorMix( a : vec3f, b : vec3f, n : f32 ) -> vec3f
{
    let aa : vec3f = a * a;
    let bb : vec3f = b * b;
    return sqrt(mix(aa, bb, n));
}

fn sminN( a : f32, b : f32, k : f32, n : f32 ) -> vec2f
{
    let h : f32 = max(k - abs(a - b), 0.0) / k;
    let m : f32 = pow(h, n) * 0.5;
    let s : f32 = m * k / n;
    if (a < b) {
        return vec2f(a - s, m);
    } else {
        return vec2f(b - s, 1.0 - m);
    }
}

fn opSmoothUnion( s1 : Surface, s2 : Surface, k : f32 ) -> Surface
{
    let smin : vec2f = sminN(s2.distance, s1.distance, k, 3.0);
    var sf : Surface;
    sf.distance = smin.x;
    sf.color = colorMix(s2.color, s1.color, smin.y);
    return sf;
}
