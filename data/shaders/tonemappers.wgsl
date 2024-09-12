// Uncharted 2 tone map
// see: http://filmicworlds.com/blog/filmic-tonemapping-operators/
fn tonemap_uncharted2_imp( color : vec3f ) -> vec3f
{
    let A = 0.15;
    let B = 0.50;
    let C = 0.10;
    let D = 0.20;
    let E = 0.02;
    let F = 0.30;
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

fn tonemap_uncharted( c : vec3f ) -> vec3f
{
    let W = 11.2;
    let color = tonemap_uncharted2_imp(c * 2.0);
    let whiteScale = 1.0 / tonemap_uncharted2_imp( vec3f(W) );
    return color * whiteScale;//LINEARtoSRGB(color * whiteScale);
}

// https://github.com/godotengine/godot/blob/aa5b6ed13e4644633baf2a8a1384c82e91c533a1/servers/rendering/renderer_rd/shaders/effects/tonemap.glsl#L197
fn tonemap_filmic(color : vec3f, white : f32) -> vec3f
{
	let exposure_bias : f32 = 2.0;
	let A : f32 = 0.22 * exposure_bias * exposure_bias; // bias baked into constants for performance
	let B : f32 = 0.30 * exposure_bias;
	let C : f32 = 0.10;
	let D : f32 = 0.20;
	let E : f32 = 0.01;
	let F : f32 = 0.30;

	let color_tonemapped : vec3f = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	let white_tonemapped : f32 = ((white * (A * white + C * B) + D * E) / (white * (A * white + B) + D * F)) - E / F;

	return color_tonemapped / white_tonemapped;
}

fn tonemap_khronos_pbr_neutral( color : vec3f ) -> vec3f
{
    var final_color : vec3f = color;

    let startCompression : f32 = 0.8 - 0.04;
    let desaturation : f32 = 0.15;

    let x : f32 = min(final_color.r, min(final_color.g, final_color.b));
    let offset : f32 = select(0.04,  x - 6.25 * x * x, x < 0.08);
    final_color -= offset;

    let peak : f32 = max(final_color.r, max(final_color.g, final_color.b));
    if (peak < startCompression) {
        return final_color;
    }

    let d : f32 = 1. - startCompression;
    let newPeak : f32 = 1. - d * d / (peak + d - startCompression);
    final_color *= newPeak / peak;

    let g : f32 = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
    return mix(final_color, vec3(1, 1, 1), g);
}