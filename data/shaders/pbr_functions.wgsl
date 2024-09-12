const PI = 3.14159265359;

fn V_SmithGGXCorrelated(NoV : f32, NoL : f32, roughness : f32) -> f32
{
    let a2 : f32 = pow(roughness, 4.0);
    let GGXV : f32 = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    let GGXL : f32 = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    let GGX : f32 = GGXV + GGXL;
    if(GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

fn D_GGX(NdotH : f32, roughness : f32) -> f32
{
    let a : f32 = NdotH * roughness;
    let k : f32 = roughness / (1.0 - NdotH * NdotH + a * a);
    return k * k * (1.0 / PI);
}

fn DistributionGGX(NdotH : f32, roughness4 : f32) -> f32 {
	let NdotH2 : f32 = NdotH * NdotH;
	var denom : f32 = (NdotH2 * (roughness4 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return roughness4 / denom;
}

// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/shaders/ibl_filtering.frag#L217
fn importance_sample_GGX(Xi : vec2f, roughness4 : f32) -> vec3f
{	
    let phi : f32 = 2.0 * PI * Xi.x;
    let cos_theta : f32 = clamp(sqrt((1.0 - Xi.y) / (1.0 + (roughness4 - 1.0) * Xi.y)), 0.0, 1.0);
    let sin_theta : f32 = sqrt(1.0 - cos_theta * cos_theta);

    let H : vec3f = vec3f(
        sin_theta * cos(phi), 
        sin_theta * sin(phi), 
        cos_theta
    );

    return H;
}

fn RadicalInverse_VdC(bits_in : u32) -> f32
{
    var bits : u32 = bits_in;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return f32(bits) * 2.3283064365386963e-10; // / 0x100000000
}

fn Hammersley(i : u32, N : u32) -> vec2f
{
    return vec2f(f32(i)/ f32(N), RadicalInverse_VdC(i));
}  

fn F_Schlick(f0 : vec3f, f90 : vec3f, VdotH : f32) -> vec3f
{
    return f0 + (f90 - f0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

fn FresnelSchlickRoughness(n_dot_v : f32, F0 : vec3f, roughness : f32) -> vec3f
{
    return F0 + (max(vec3f(1.0 - roughness), F0) - F0) * pow(1.0 - n_dot_v, 5.0);
}

//  https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
fn BRDF_specularGGX(f0 : vec3f, f90 : vec3f, alpha_roughness : f32, specular_weight : f32, VdotH : f32, NdotL : f32, NdotV : f32, NdotH : f32) -> vec3f
{
    let F : vec3f = F_Schlick(f0, f90, VdotH);
    let Vis : f32 = V_SmithGGXCorrelated(NdotV, NdotL, alpha_roughness);
    let D : f32 = D_GGX(NdotH, alpha_roughness);

    return specular_weight * F * Vis * D;
}

//https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB
fn BRDF_lambertian(f0 : vec3f, f90 : vec3f, diffuse_color : vec3f, specular_weight : f32, VdotH : f32) -> vec3f
{
    // see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
    return (1.0 - specular_weight * F_Schlick(f0, f90, VdotH)) * (diffuse_color / PI);
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
