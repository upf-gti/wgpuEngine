
const LIGHT_UNDEFINED   = 0;
const LIGHT_DIRECTIONAL = 1;
const LIGHT_OMNI        = 2;
const LIGHT_SPOT        = 3;

struct Light
{
    view_proj : mat4x4f,

    position : vec3f,
    ltype : u32,

    color : vec3f,
    intensity : f32,

    direction : vec3f,
    range : f32,

    shadow_bias : f32,
    cast_shadows : i32,
    inner_cone_cos : f32,
    outer_cone_cos : f32
};

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#range-property
fn get_range_attenuation(range : f32, dist : f32) -> f32
{
    if (range < 0.0)
    {
        // negative range means unlimited
        return 1.0 / pow(dist, 2.0);
    }
    return max(min(1.0 - pow(dist / range, 4.0), 1.0), 0.0) / pow(dist, 2.0);
}

// https://github.com/KhronosGroup/glTF/blob/master/extensions/2.0/Khronos/KHR_lights_punctual/README.md#inner-and-outer-cone-angles
fn get_spot_attenuation(point_to_light : vec3f, spot_direction : vec3f, outer_cone_cos : f32, inner_cone_cos : f32) -> f32
{
    // These two values can be calculated on the CPU and passed into the shader
    let light_angle_scale : f32 = 1.0 / max(0.001, inner_cone_cos - outer_cone_cos);
    let light_angle_offset : f32 = -outer_cone_cos * light_angle_scale;

    let cd : f32 = dot(-normalize(spot_direction), normalize(point_to_light));
    var angular_attenuation : f32 = clamp(cd * light_angle_scale + light_angle_offset, 0.0, 1.0);
    angular_attenuation *= angular_attenuation;

    return angular_attenuation;

    // let actual_cos : f32 = dot(normalize(-spot_direction), normalize(point_to_light));
    // if (actual_cos > outer_cone_cos)
    // {
    //     if (actual_cos < inner_cone_cos)
    //     {
    //         let angular_attenuation : f32 = (actual_cos - outer_cone_cos) / (inner_cone_cos - outer_cone_cos);
    //         return angular_attenuation * angular_attenuation;
    //     }
    //     return 1.0;
    // }
    // return 0.0;
}

fn get_light_intensity(light : Light, point_to_light : vec3f) -> vec3f
{
    var range_attenuation : f32 = 1.0;
    var spot_attenuation : f32 = 1.0;

    if (light.ltype != LIGHT_DIRECTIONAL) {

        range_attenuation = get_range_attenuation(light.range, length(point_to_light));

        if (light.ltype == LIGHT_SPOT) {
            spot_attenuation = get_spot_attenuation(point_to_light, light.direction, light.outer_cone_cos, light.inner_cone_cos);
        }
    }

    return range_attenuation * spot_attenuation * light.intensity * light.color;
}

fn get_direct_light( m : ptr<function, PbrMaterial> ) -> vec3f
{
    var f_diffuse : vec3f = vec3f(0.0);
    var f_specular : vec3f = vec3f(0.0);

    var n : vec3f = m.normal;
    var v : vec3f = m.view_dir;

    let num_lights_clamped : u32 = clamp(num_lights, 0u, MAX_LIGHTS);

    for (var light_idx : u32 = 0; light_idx < num_lights_clamped; light_idx++)
    {
        var light : Light = lights[light_idx];

        var point_to_light : vec3f;

        if ( light.ltype == LIGHT_DIRECTIONAL ) {
            point_to_light = -light.direction;
        }
        else {
            point_to_light = light.position - m.pos;
        }

        // BSTF
        let l : vec3f = normalize(point_to_light);   // Direction from surface point to light
        let h : vec3f = normalize(l + v);          // Direction of the vector between l and v, called halfway vector

        let NdotL : f32 = clamp(dot(n, l), 0.0, 1.0);
        let NdotH : f32 = clamp(dot(n, h), 0.0, 1.0);
        let LdotH : f32 = clamp(dot(l, h), 0.0, 1.0);
        let VdotH : f32 = clamp(dot(v, h), 0.0, 1.0);

        let light_space = light.view_proj * vec4f(m.pos, 1.0);
        let proj = light_space.xyz / light_space.w;
        var uv : vec2f = proj.xy * vec2f(0.5, -0.5) + vec2f(0.5);
        var depth : f32 = proj.z;

        // Percentage-closer filtering. Sample texels in the region
        // to smooth the result.
        // https://webgpu.github.io/webgpu-samples/?sample=shadowMapping#fragment.wgsl

        var visibility : f32 = 0.0;
        let i_map_size = 1.0 / 1024.0; // 1024x1024 shadow map

        for (var y = -1; y <= 1; y++) {
            for (var x = -1; x <= 1; x++) {
                let offset : vec2f = vec2f(f32(x), f32(y)) * i_map_size;
                visibility += textureSampleCompare(
                    lights_shadow_maps,
                    shadow_sampler,
                    uv + offset,
                    light_idx,
                    depth + light.shadow_bias
                );
            }
        }

        visibility /= 9.0;

        if(light.cast_shadows == 0) {
            visibility = 1.0;
        }

        if (NdotL > 0.0)
        {
            // Calculation of analytical light
            // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB

            let intensity : vec3f = visibility * NdotL * get_light_intensity(light, point_to_light);
            f_diffuse += intensity * BRDF_lambertian(m.f0, m.f90, m.diffuse, m.specular_weight, VdotH);
            f_specular += intensity * BRDF_specularGGX(m.f0, m.f90, m.roughness * m.roughness, m.specular_weight, VdotH, NdotL, m.n_dot_v, NdotH);
        }
    }

    return f_diffuse + f_specular;
}

// https://github.com/KhronosGroup/glTF-Sample-Renderer/blob/fe6ee033f6ad33e21085452a23e284d589c18271/source/Renderer/shaders/ibl.glsl
fn get_indirect_light( m : ptr<function, PbrMaterial> ) -> vec3f
{
    // IBL
    // Specular + Diffuse

    var irradiance : vec3f = get_ibl_diffuse_light(m.normal);

#ifdef ANISOTROPY_MATERIAL
    var radiance : vec3f = get_ibl_radiance_anisotropy(m);
#else
    var radiance : vec3f = get_ibl_radiance_ggx(m.normal, m.view_dir, m.roughness);
#endif

    // White furnace test
    // radiance = vec3f(1.0);
    // irradiance = vec3f(1.0);

    // Combined dielectric and metallic IBL
    // https://github.com/mrdoob/three.js/blob/c48f842f5d0fdff950c1a004803e659171bbcb85/src/renderers/shaders/ShaderChunk/lights_physical_pars_fragment.glsl.js#L543

    // let brdf_coords : vec2f = clamp(vec2f(n_dot_v, roughness), vec2f(0.0, 0.0), vec2f(1.0, 1.0));
    // let brdf_lut : vec2f = textureSampleLevel(brdf_lut_texture, sampler_clamp, brdf_coords, 0.0).rg;
    // let fss_ess : vec3f = m.f0 * brdf_lut.x + brdf_lut.y;
    // let Ess : f32 = brdf_lut.x + brdf_lut.y;
    // let Ems : f32 = 1.0 - Ess;
    // let Favg : vec3f = m.f0 + (1.0 - m.f0) * 0.047619; // 1 / 21
    // let Fms : vec3f = fss_ess * Favg / (1.0 - Ems * Favg);
    // let single_scatter = fss_ess;
    // let multi_scatter = Fms * Ems;

    // let total_scattering : vec3f = single_scatter + multi_scatter;
    // var diffuse : vec3f = irradiance * (m.diffuse / PI) + m.diffuse * (1.0 - max(max(total_scattering.r, total_scattering.g), total_scattering.b)) * irradiance;
    // var specular : vec3f = single_scatter * radiance + multi_scatter * irradiance;
    // var total_indirect : vec3f = diffuse + specular;

    // Separate Fresnel for metallic and dielectric materials
    // https://github.com/KhronosGroup/glTF-Sample-Renderer/blob/3dc1bd9bae75f67c1414bbdaf1bdfddb89aa39d6/source/Renderer/shaders/pbr.frag

    var f_metal_fresnel_ibl : vec3f = get_ibl_ggx_fresnel(m, m.albedo);
    var f_metal_brdf_ibl : vec3f = f_metal_fresnel_ibl * radiance;

    let f_diffuse : vec3f = irradiance * m.albedo;
    var f_dielectric_fresnel_ibl : vec3f = get_ibl_ggx_fresnel(m, m.f0_dielectric);
    var f_dielectric_brdf_ibl : vec3f = mix(f_diffuse, radiance, f_dielectric_fresnel_ibl);

#ifdef IRIDESCENCE_MATERIAL
    let iridescence_fresnel_dielectric : vec3f = eval_iridescence(1.0, m.iridescence_ior, m.n_dot_v, m.iridescence_thickness, vec3f(0.04));
    let iridescence_fresnel_metallic : vec3f = eval_iridescence(1.0, m.iridescence_ior, m.n_dot_v, m.iridescence_thickness, m.albedo);
    let iridescence_fresnel : vec3f = mix(iridescence_fresnel_dielectric, iridescence_fresnel_metallic, m.metallic);

    if (m.iridescence_thickness == 0.0) {
        m.iridescence_factor = 0.0;
    }

    var mixed : vec3f = vec3f(
        mix(f_diffuse.r, radiance.r, iridescence_fresnel_dielectric.r),
        mix(f_diffuse.g, radiance.g, iridescence_fresnel_dielectric.g),
        mix(f_diffuse.b, radiance.b, iridescence_fresnel_dielectric.b)
    );

    f_metal_brdf_ibl = mix(f_metal_brdf_ibl, radiance * iridescence_fresnel_metallic, m.iridescence_factor);
    f_dielectric_brdf_ibl = mix(f_dielectric_brdf_ibl, mixed, m.iridescence_factor);

#endif

    var total_indirect : vec3f = mix(f_dielectric_brdf_ibl, f_metal_brdf_ibl, m.metallic);

#ifdef CLEARCOAT_MATERIAL
    // Only for IBL
    let cc_normal_dot_v : f32 = clamp(dot(m.clearcoat_normal, m.view_dir), 0.0, 1.0);
    m.clearcoat_fresnel = F_Schlick(m.clearcoat_f0, m.clearcoat_f90, cc_normal_dot_v);
    let clearcoat_brdf : vec3f = get_ibl_radiance_ggx(m.clearcoat_normal, m.view_dir, m.clearcoat_roughness);
    total_indirect = mix(total_indirect, clearcoat_brdf, m.clearcoat_factor * m.clearcoat_fresnel);
#endif

    // Add AO
    return total_indirect * m.ao;
}

fn get_ibl_diffuse_light( n : vec3f ) -> vec3f
{
    let max_mipmap : f32 = 5.0;

    var irradiance : vec3f = textureSampleLevel(irradiance_texture, sampler_clamp, n, max_mipmap).rgb * camera_data.ibl_intensity;

    return irradiance;
}

fn get_ibl_radiance_ggx( n : vec3f, v : vec3f, roughness : f32 ) -> vec3f
{
    let n_dot_v = clamp(dot(n, v), 0.0, 1.0);
    let max_mipmap : f32 = 5.0;
    let lod : f32 = roughness * max_mipmap;

    // Mixing the reflection with the normal is more accurate and keeps rough objects from gathering light from behind their tangent plane
    // https://github.com/mrdoob/three.js/blob/c48f842f5d0fdff950c1a004803e659171bbcb85/src/renderers/shaders/ShaderChunk/envmap_physical_pars_fragment.glsl.js#L29
    let reflection : vec3f = normalize(reflect(-v, n));
    let reflected_dir : vec3f = normalize( mix( reflection, n, roughness * roughness) );

    var specular_sample : vec3f = textureSampleLevel(irradiance_texture, sampler_clamp, reflected_dir, lod).rgb * camera_data.ibl_intensity;

    return specular_sample;
}

fn get_ibl_ggx_fresnel( m : ptr<function, PbrMaterial>, F0 : vec3f ) -> vec3f
{
    // see https://bruop.github.io/ibl/#single_scattering_results at Single Scattering Results
    // Roughness dependent fresnel, from Fdez-Aguera
    // Multiple scattering (from: http://www.jcgt.org/published/0008/01/03/)
    let brdf_coords : vec2f = clamp(vec2f(m.n_dot_v, m.roughness), vec2f(0.0), vec2f(1.0));
    let brdf_lut : vec2f = textureSampleLevel(brdf_lut_texture, sampler_clamp, brdf_coords, 0.0).rg;

    // alternative for brdf_lut to avoid texture sample
    // let c0 : vec4f = vec4f(-1.0, -0.0275, -0.572, 0.022);
    // let c1 : vec4f = vec4f(1.0, 0.0425, 1.04, -0.04);
    // let r : vec4f = roughness * c0 + c1;
    // let a004 : f32 = min(r.x * r.x, exp2(-9.28 * n_dot_v)) * r.x + r.y;
    // let brdf_lut : vec2f = vec2f(-1.04, 1.04) * a004 + r.zw;

    // var Fr : vec3f = max(vec3f(1.0 - m.roughness), F0) - F0;
    var k_S : vec3f = F0;// + Fr * pow(1.0 - m.n_dot_v, 5.0);
    var FssEss : vec3f = m.specular_weight * (k_S * brdf_lut.x + brdf_lut.y);

    // Multiple scattering, from Fdez-Aguera
    var Ems : f32 = (1.0 - (brdf_lut.x + brdf_lut.y));
    var F_avg : vec3f = m.specular_weight * (F0 + (1.0 - F0) / 21.0);
    var FmsEms : vec3f = Ems * FssEss * F_avg / (1.0 - F_avg * Ems);

    return FssEss + FmsEms;
}

#ifdef ANISOTROPY_MATERIAL
fn get_ibl_radiance_anisotropy( m : ptr<function, PbrMaterial> ) -> vec3f
{
    let n : vec3f = m.normal;
    let v : vec3f = m.view_dir;
    let roughness : f32 = m.roughness;
    let anisotropy : f32 = m.anisotropy_factor;
    let direction : vec3f = m.anisotropy_bitangent;
    let n_dot_v : f32 = m.n_dot_v;

    let tangent_roughness : f32 = mix(roughness, 1.0, anisotropy * anisotropy);
    let anisotropic_tangent : vec3f = cross(direction, v);
    let anisotropic_normal   = cross(anisotropic_tangent, direction);
    let bend_factor : f32 = 1.0 - anisotropy * (1.0 - roughness);
    let bend_factor_pow4 : f32 = bend_factor * bend_factor * bend_factor * bend_factor;
    let bent_normal : vec3f = normalize(mix(anisotropic_normal, n, bend_factor_pow4));

    let max_mipmap : f32 = 5.0;
    let lod : f32 = roughness * max_mipmap;

    let reflection : vec3f = normalize(reflect(-v, bent_normal));

    var specular_sample : vec3f = textureSampleLevel(irradiance_texture, sampler_clamp, reflection, lod).rgb * camera_data.ibl_intensity;

    return specular_sample;
}
#endif