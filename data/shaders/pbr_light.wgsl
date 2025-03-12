
const LIGHT_UNDEFINED   = 0;
const LIGHT_DIRECTIONAL = 1;
const LIGHT_OMNI        = 2;
const LIGHT_SPOT        = 3;

struct Light
{
    position : vec3f,
    ltype : u32,

    color : vec3f,
    intensity : f32,

    direction : vec3f,
    range : f32,

    // spots
    dummy: vec2f,
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

    for (var i : u32 = 0; i < num_lights_clamped; i++)
    {
        var light : Light = lights[i];

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

        if (NdotL > 0.0 || m.n_dot_v > 0.0)
        {
            // Calculation of analytical light
            // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB

            let intensity : vec3f = get_light_intensity(light, point_to_light);
            f_diffuse += intensity * NdotL * BRDF_lambertian(m.f0, m.f90, m.diffuse, m.specular_weight, VdotH);
            f_specular += intensity * NdotL * BRDF_specularGGX(m.f0, m.f90, m.roughness * m.roughness, m.specular_weight, VdotH, NdotL, m.n_dot_v, NdotH);
        }
    }

    return f_diffuse + f_specular;
}

// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/Renderer/shaders/ibl.glsl
fn get_indirect_light( m : ptr<function, PbrMaterial> ) -> vec3f
{
    let max_mipmap : f32 = 5.0;

    let roughness : f32 = m.roughness;
    let n_dot_v : f32 = m.n_dot_v;

    let lod : f32 = roughness * max_mipmap;

    // IBL
    // https://github.com/mrdoob/three.js/blob/c48f842f5d0fdff950c1a004803e659171bbcb85/src/renderers/shaders/ShaderChunk/lights_physical_pars_fragment.glsl.js#L543
    // Specular + Diffuse

    let fresnel : vec3f = FresnelSchlickRoughness(n_dot_v, m.f0, roughness);

    // Specular color

    let brdf_coords : vec2f = clamp(vec2f(n_dot_v, roughness), vec2f(0.0, 0.0), vec2f(1.0, 1.0));
    let brdf_lut : vec2f = textureSampleLevel(brdf_lut_texture, sampler_clamp, brdf_coords, 0.0).rg;

    // Mixing the reflection with the normal is more accurate and keeps rough objects from gathering light from behind their tangent plane
    // https://github.com/mrdoob/three.js/blob/c48f842f5d0fdff950c1a004803e659171bbcb85/src/renderers/shaders/ShaderChunk/envmap_physical_pars_fragment.glsl.js#L29
    let reflected_dir : vec3f = normalize( mix( m.reflected_dir, m.normal, roughness * roughness) );
    
    let radiance : vec3f = textureSampleLevel(irradiance_texture, sampler_clamp, reflected_dir, lod).rgb * camera_data.ibl_intensity;
    let irradiance : vec3f = textureSampleLevel(irradiance_texture, sampler_clamp, m.normal, max_mipmap).rgb * camera_data.ibl_intensity * PI;

    let cosine_weight_irradiance : vec3f = irradiance / PI;

    // alternative for brdf_lut to avoid texture sample
    // let c0 : vec4f = vec4f(-1.0, -0.0275, -0.572, 0.022);
    // let c1 : vec4f = vec4f(1.0, 0.0425, 1.04, -0.04);
    // let r : vec4f = roughness * c0 + c1;
    // let a004 : f32 = min(r.x * r.x, exp2(-9.28 * n_dot_v)) * r.x + r.y;
    // let brdf_lut : vec2f = vec2f(-1.04, 1.04) * a004 + r.zw;

    let fss_ess : vec3f = m.f0 * brdf_lut.x + brdf_lut.y;

    // Multiple scattering (from: http://www.jcgt.org/published/0008/01/03/)
    let Ess : f32 = brdf_lut.x + brdf_lut.y;
    let Ems : f32 = 1.0 - Ess;
    let Favg : vec3f = m.f0 + (1.0 - m.f0) * 0.047619; // 1 / 21
    let Fms : vec3f = fss_ess * Favg / (1.0 - Ems * Favg);

    let single_scatter = fss_ess;
    let multi_scatter = Fms * Ems;

    // Diffuse color

    let total_scattering : vec3f = single_scatter + multi_scatter;

    let diffuse : vec3f = cosine_weight_irradiance * (m.diffuse / PI) + m.diffuse * (1.0 - max(max(total_scattering.r, total_scattering.g), total_scattering.b)) * cosine_weight_irradiance;
    let specular : vec3f = single_scatter * radiance + multi_scatter * cosine_weight_irradiance;

    // Combine factors and add AO
    return (diffuse + specular) * m.ao;
}
