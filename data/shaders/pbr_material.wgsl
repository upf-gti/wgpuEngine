struct PbrMaterial
{
    pos : vec3f,
    normal_g : vec3f,
    normal : vec3f,
    albedo : vec3f,
    emissive : vec3f,
    f0 : vec3f,
    f90 : vec3f,
    ior: f32,
    diffuse : vec3f,
    specular_weight: f32,
    metallic : f32,
    roughness : f32,
    ao : f32,

    // KHR_materials_clearcoat
    clearcoat_f0 : vec3f,
    clearcoat_f90 : vec3f,
    clearcoat_factor : f32,
    clearcoat_normal: vec3f,
    clearcoat_roughness : f32,

    // KHR_materials_iridescence
    iridescence_factor : f32,
    iridescence_ior : f32,
    iridescence_thickness : f32,

    n_dot_v : f32,
    view_dir : vec3f,
    reflected_dir : vec3f
};