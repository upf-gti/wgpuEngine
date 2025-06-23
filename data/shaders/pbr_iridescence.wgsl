// https://github.com/KhronosGroup/glTF-Sample-Renderer/blob/3dc1bd9bae75f67c1414bbdaf1bdfddb89aa39d6/source/Renderer/shaders/iridescence.glsl

// XYZ to sRGB color space
const XYZ_TO_REC709: mat3x3f = mat3x3f(
    vec3f( 3.2404542, -1.5371385, -0.4985314),
    vec3f(-0.9692660,  1.8760108,  0.0415560),
    vec3f( 0.0556434, -0.2040259,  1.0572252)
);

// Assume air interface for top
// Note: We don't handle the case fresnel0 == 1
fn Fresnel0ToIor(fresnel0 : vec3f) -> vec3f {
    let sqrtF0 : vec3f = sqrt(fresnel0);
    return (vec3f(1.0) + sqrtF0) / (vec3f(1.0) - sqrtF0);
}

// Conversion FO/IOR
fn IorToFresnel0_vec(transmittedIor: vec3f, incidentIor: f32) -> vec3f {
    return sq3((transmittedIor - vec3f(incidentIor)) / (transmittedIor + vec3f(incidentIor)));
}

// ior is a value between 1.0 and 3.0. 1.0 is air interface
fn IorToFresnel0(transmittedIor: f32, incidentIor: f32) -> f32 {
    return sq((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

fn F_Schlick_float(F0: f32, cosTheta: f32) -> f32 {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

fn F_Schlick_vec(F0: vec3f, cosTheta: f32) -> vec3f {
    return F0 + (vec3f(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// Fresnel equations for dielectric/dielectric interfaces.
// Ref: https://belcour.github.io/blog/research/2017/05/01/brdf-thin-film.html
// Evaluation XYZ sensitivity curves in Fourier space
fn eval_sensitivity(OPD: f32, shift: vec3f) -> vec3f {
    let phase = 2.0 * M_PI * OPD * 1.0e-9;

    let val = vec3f(5.4856e-13, 4.4201e-13, 5.2481e-13);
    let pos = vec3f(1.6810e+06, 1.7953e+06, 2.2084e+06);
    let _var = vec3f(4.3278e+09, 9.3046e+09, 6.6121e+09);

    var xyz = val * sqrt(2.0 * M_PI * _var) * cos(pos * phase);// * exp(-sq(phase) * _var);
    xyz.x += 9.7470e-14 * sqrt(2.0 * M_PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift.x) * exp(-4.5282e+09 * sq(phase));
    xyz = xyz / 1.0685e-7;

    return XYZ_TO_REC709 * xyz;
}

fn eval_iridescence( outsideIOR: f32, eta2: f32, cosTheta1: f32, thinFilmThickness: f32, baseF0: vec3f ) -> vec3f
{
    var I: vec3f;

    let iridescenceIor = mix(outsideIOR, eta2, smoothstep(0.0, 0.03, thinFilmThickness));
    let sinTheta2Sq = sq(outsideIOR / iridescenceIor) * (1.0 - sq(cosTheta1));
    let cosTheta2Sq = 1.0 - sinTheta2Sq;

    if (cosTheta2Sq < 0.0) {
        return vec3f(1.0);
    }

    let cosTheta2 = sqrt(cosTheta2Sq);

    let R0 : f32 = IorToFresnel0(iridescenceIor, outsideIOR);
    let R12 = F_Schlick_float(R0, cosTheta1);
    let T121 = 1.0 - R12;
    let phi12 = select(0.0, M_PI, iridescenceIor < outsideIOR);
    let phi21 = M_PI - phi12;

    let baseIOR = Fresnel0ToIor(clamp(baseF0, vec3f(0.0), vec3f(0.9999)));
    let R1 = IorToFresnel0_vec(baseIOR, iridescenceIor);
    let R23 = F_Schlick_vec(R1, cosTheta2);

    var phi23 = vec3f(0.0);
    if(baseIOR.x < iridescenceIor) { phi23.x = M_PI; }
    if(baseIOR.y < iridescenceIor) { phi23.y = M_PI; }
    if(baseIOR.z < iridescenceIor) { phi23.z = M_PI; }

    let OPD = 2.0 * iridescenceIor * thinFilmThickness * cosTheta2;
    let phi = vec3f(phi21) + phi23;

    let R123 = clamp(vec3f(R12) * R23, vec3f(1e-5), vec3f(0.9999));
    let r123 = sqrt(R123);
    let Rs = sq(T121) * R23 / (vec3f(1.0) - R123);

    let C0 = vec3f(R12) + Rs;
    I = C0;

    var Cm = Rs - vec3f(T121);
    for (var m = 1; m <= 2; m = m + 1) {
        Cm *= r123;
        let Sm = 2.0 * eval_sensitivity(f32(m) * OPD, f32(m) * phi);
        I = I + Cm * Sm;
    }

    return max(I, vec3f(0.0));
}