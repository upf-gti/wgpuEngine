
const NORMAL_UV_TRANSFORM                   = 1;
const METALLIC_ROUGHNESS_UV_TRANSFORM       = 2;
const EMISSIVE_UV_TRANSFORM                 = 3;
const OCCLUSION_UV_TRANSFORM                = 4;
const CLEARCOAT_UV_TRANSFORM                = 5;
const CLEARCOAT_ROUGHNESS_UV_TRANSFORM      = 6;
const CLEARCOAT_NORMAL_UV_TRANSFORM         = 7;
const IRIDESCENCE_UV_TRANSFORM              = 8;
const IRIDESCENCE_THICKNESS_UV_TRANSFORM    = 9;
const ANISOTROPY_UV_TRANSFORM               = 10;

#ifdef ALBEDO_TEXTURE
fn get_albedo_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_DIFFUSE_UV_TRANSFORM
    uv = uv_transform_data[DIFFUSE_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

#ifndef UNLIT_MATERIAL

#ifdef NORMAL_TEXTURE
fn get_normal_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_NORMAL_UV_TRANSFORM
    uv = uv_transform_data[NORMAL_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

fn get_normal_info( m : ptr<function, PbrMaterial>, in : VertexOutput, is_front_facing : bool )
{
    m.normal_g = normalize(in.normal);

    if (!is_front_facing) {
        m.normal_g = -m.normal_g;
    }

#ifdef NORMAL_TEXTURE
    let normal_uv : vec2f = get_normal_uv(in.uv);
    var normal_color = textureSample(normal_texture, sampler_2d, normal_uv).rgb * 2.0 - 1.0;
    normal_color *= vec3f(normal_scale, normal_scale, 1.0);

#ifdef HAS_TANGENTS
    let TBN : mat3x3f = mat3x3f(in.tangent, in.bitangent, m.normal_g);
    m.normal = normalize(TBN * normalize(normal_color));
#else
    m.normal = perturb_normal(m.normal_g, m.view_dir, normal_uv, normal_color);
#endif
#else
    m.normal = m.normal_g;
#endif // NORMAL_TEXTURE

    m.n_dot_v = clamp(dot(m.normal, m.view_dir), 0.0, 1.0);
}

#ifdef METALLIC_ROUGHNESS_TEXTURE
fn get_metallic_roughness_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_METALLIC_ROUGHNESS_UV_TRANSFORM
    uv = uv_transform_data[METALLIC_ROUGHNESS_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

#ifdef EMISSIVE_TEXTURE
fn get_emissive_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_EMISSIVE_UV_TRANSFORM
    uv = uv_transform_data[EMISSIVE_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

#ifdef OCLUSSION_TEXTURE
fn get_occlusion_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_OCCLUSION_UV_TRANSFORM
    uv = uv_transform_data[OCCLUSION_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

#ifdef CLEARCOAT_MATERIAL

#ifdef CLEARCOAT_TEXTURE
fn get_clearcoat_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_CLEARCOAT_UV_TRANSFORM
    uv = uv_transform_data[CLEARCOAT_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

#ifdef CLEARCOAT_ROUGHNESS_TEXTURE
fn get_clearcoat_roughness_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_CLEARCOAT_ROUGHNESS_UV_TRANSFORM
    uv = uv_transform_data[CLEARCOAT_ROUGHNESS_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

#ifdef CLEARCOAT_NORMAL_TEXTURE
fn get_clearcoat_normal_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_CLEARCOAT_NORMAL_UV_TRANSFORM
    uv = uv_transform_data[CLEARCOAT_NORMAL_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

fn get_clearcoat_info( m : ptr<function, PbrMaterial>, in : VertexOutput )
{
    m.clearcoat_factor = clearcoat_data.x;
    m.clearcoat_roughness = clearcoat_data.y;
    m.clearcoat_f0 = vec3f(pow((m.ior - 1.0) / (m.ior + 1.0), 2.0));
    m.clearcoat_f90 = vec3f(1.0);

#ifdef CLEARCOAT_TEXTURE
    let clearcoat_uv : vec2f = get_clearcoat_uv(in.uv);
    let clearcoat_sample : vec4f = textureSample(clearcoat_texture, sampler_2d, clearcoat_uv);
    m.clearcoat_factor *= clearcoat_sample.r;
#endif

#ifdef CLEARCOAT_ROUGHNESS_TEXTURE
    let clearcoat_roughness_uv : vec2f = get_clearcoat_roughness_uv(in.uv);
    let clearcoat_sample_roughness : vec4f = textureSample(clearcoat_roughness_texture, sampler_2d, clearcoat_roughness_uv);
    m.clearcoat_roughness *= clearcoat_sample_roughness.g;
#endif

#ifdef CLEARCOAT_NORMAL_TEXTURE
    let clearcoat_normal_uv : vec2f = get_clearcoat_normal_uv(in.uv);
    var clearcoat_normal : vec3f = textureSample(clearcoat_normal_texture, sampler_2d, clearcoat_normal_uv).rgb * 2.0 - vec3(1.0);

#ifdef HAS_TANGENTS
    let TBN : mat3x3f = mat3x3f(in.tangent, in.bitangent, m.normal_g);
    m.clearcoat_normal = normalize(TBN * normalize(clearcoat_normal));
#else
    m.clearcoat_normal = perturb_normal(m.normal_g, m.view_dir, clearcoat_normal_uv, clearcoat_normal);
#endif // HAS_TANGENTS
#else
    m.clearcoat_normal = m.normal_g;
#endif // CLEARCOAT_NORMAL_TEXTURE

    m.clearcoat_roughness = clamp(m.clearcoat_roughness, 0.04, 1.0);
}

#endif // CLEARCOAT_MATERIAL

#ifdef IRIDESCENCE_MATERIAL

#ifdef IRIDESCENCE_TEXTURE
fn get_iridescence_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_IRIDESCENCE_UV_TRANSFORM
    uv = uv_transform_data[IRIDESCENCE_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

#ifdef IRIDESCENCE_THICKNESS_TEXTURE
fn get_iridescence_thickness_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_IRIDESCENCE_THICKNESS_UV_TRANSFORM
    uv = uv_transform_data[IRIDESCENCE_THICKNESS_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

fn get_iridescence_info( m : ptr<function, PbrMaterial>, in : VertexOutput )
{
    m.iridescence_factor = iridescence_data.x;
    m.iridescence_ior = iridescence_data.y;
    m.iridescence_thickness = iridescence_data.w;

#ifdef IRIDESCENCE_TEXTURE
    let iridescence_uv : vec2f = get_iridescence_uv(in.uv);
    m.iridescence_factor *= textureSample(iridescence_texture, sampler_2d, iridescence_uv).r;
#endif

#ifdef IRIDESCENCE_THICKNESS_TEXTURE
    let iridescence_thickness_uv : vec2f = get_iridescence_thickness_uv(in.uv);
    let thickness_sampled : f32 = textureSample(iridescence_thickness_texture, sampler_2d, iridescence_thickness_uv).g;
    m.iridescence_thickness = mix(iridescence_data.z, iridescence_data.w, thickness_sampled);
#endif
}

#endif // IRIDESCENCE_MATERIAL

#ifdef ANISOTROPY_MATERIAL

#ifdef ANISOTROPY_TEXTURE
fn get_anisotropy_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_ANISOTROPY_UV_TRANSFORM
    uv = uv_transform_data[ANISOTROPY_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

fn get_anisotropy_info( m : ptr<function, PbrMaterial>, in : VertexOutput )
{
    var direction : vec2f = vec2f(1.0, 0.0);
    var anisotropy_factor : f32 = 1.0;
    
#ifdef ANISOTROPY_TEXTURE
    let anisotropy_uv : vec2f = get_anisotropy_uv(in.uv);
    let anisotropy_sample : vec3f = textureSample(anisotropy_texture, sampler_2d, anisotropy_uv).xyz;
    direction = anisotropy_sample.xy * 2.0 - vec2f(1.0);
    anisotropy_factor = anisotropy_sample.z;
#endif

    let direction_rotation : vec2f = anisotropy_data.xy; // cos(theta), sin(theta)
    let rotation_matrix : mat2x2f = mat2x2f(direction_rotation.x, direction_rotation.y, -direction_rotation.y, direction_rotation.x);
    direction = rotation_matrix * direction.xy;

    m.anisotropy_tangent = mat3x3f(in.tangent, in.bitangent, m.normal) * normalize(vec3f(direction, 0.0));
    m.anisotropy_bitangent = cross(m.normal_g, m.anisotropy_tangent);
    m.anisotropy_factor = clamp(anisotropy_data.z * anisotropy_factor, 0.0, 1.0);
}

#endif // ANISOTROPY_MATERIAL

#ifdef TRANSMISSION_MATERIAL

#ifdef TRANSMISSION_TEXTURE
fn get_transmission_uv( in_uv: vec2f ) -> vec2f {
    var uv : vec4f = vec4f(in_uv, 1.0, 0.0);
#ifdef HAS_TRANSMISSION_UV_TRANSFORM
    uv = uv_transform_data[TRANSMISSION_UV_TRANSFORM] * uv;
#endif
    return uv.xy;
}
#endif

fn get_transmission_info( m : ptr<function, PbrMaterial>, in : VertexOutput )
{
    m.transmission_factor = transmission_factor;

#ifdef TRANSMISSION_TEXTURE
    let transmission_uv : vec2f = get_transmission_uv(in.uv);
    let transmission_sample : vec3f = textureSample(transmission_texture, sampler_2d, transmission_uv).xyz;
    m.transmission_factor *= transmission_sample.r;
#endif
}

#endif // TRANSMISSION_MATERIAL

#endif // UNLIT_MATERIAL