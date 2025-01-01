struct PbrMaterial
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
    n_dot_v : f32,
    view_dir : vec3f,
    reflected_dir : vec3f
};