
// Lights

#define MAX_LIGHTS

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

struct LitMaterial
{
    pos : vec3f,
    normal : vec3f,
    albedo : vec3f,
    emissive : vec3f,
    f0 : vec3f,
    f90 : vec3f,
    c_diff : vec3f,
    specular_weight: f32,
    metallic : f32,
    roughness : f32,
    ao : f32,
    view_dir : vec3f,
    reflected_dir : vec3f
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

fn get_direct_light( m : LitMaterial ) -> vec3f
{
    var f_diffuse : vec3f = vec3f(0.0);
    var f_specular : vec3f = vec3f(0.0);

    var n : vec3f = normalize(m.normal);
    var v : vec3f = normalize(m.view_dir);

    let NdotV : f32 = clamp(dot(n, v), 0.0, 1.0);

    for (var i : u32 = 0; i < MAX_LIGHTS; i++)
    {
        if(i >= num_lights) {
            break;
        }

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

        if (NdotL > 0.0 || NdotV > 0.0)
        {
            // Calculation of analytical light
            // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#acknowledgments AppendixB

            let intensity : vec3f = get_light_intensity(light, point_to_light);
            f_diffuse += intensity * NdotL * BRDF_lambertian(m.f0, m.f90, m.c_diff, m.specular_weight, VdotH);
            f_specular += intensity * NdotL * BRDF_specularGGX(m.f0, m.f90, m.roughness * m.roughness, m.specular_weight, VdotH, NdotL, NdotV, NdotH);
        }
    }

    return f_diffuse + f_specular;
}

// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/main/source/Renderer/shaders/ibl.glsl
fn get_indirect_light( m : LitMaterial ) -> vec3f
{
    let n_dot_v : f32 = clamp(dot(m.normal, m.view_dir), 0.0, 1.0);

    let max_mipmap : f32 = 5.0;

    let lod : f32 = m.roughness * max_mipmap;

    // IBL
    // Specular + Diffuse

    // Specular color

    let brdf_coords : vec2f = clamp(vec2f(n_dot_v, m.roughness), vec2f(0.0, 0.0), vec2f(1.0, 1.0));
    let brdf_lut : vec2f = textureSampleLevel(brdf_lut_texture, sampler_clamp, brdf_coords, 0.0).rg;
    
    let specular_sample : vec3f = textureSampleLevel(irradiance_texture, sampler_clamp, m.reflected_dir, lod).rgb;

    let k_s : vec3f = FresnelSchlickRoughness(n_dot_v, m.f0, m.roughness);
    let fss_ess : vec3f = (k_s * brdf_lut.x + brdf_lut.y);

    let specular : vec3f = specular_sample * fss_ess;

    // Diffuse sample: get last prefiltered mipmap
    let irradiance : vec3f = textureSampleLevel(irradiance_texture, sampler_clamp, m.normal, max_mipmap).rgb;

    // Diffuse color
    let ems : f32 = (1.0 - (brdf_lut.x + brdf_lut.y));
    let f_avg : vec3f = (m.f0 + (1.0 - m.f0) / 21.0);
    let fms_ems : vec3f = ems * fss_ess * f_avg / (1.0 - f_avg * ems);
    var diffuse : vec3f = m.c_diff * (1.0 - fss_ess + fms_ems);
    diffuse = (fms_ems + diffuse) * irradiance;

    // Combine factors and add AO
    return (diffuse + specular) * m.ao;
}
