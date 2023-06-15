#include raymarching_functions.wgsl

struct sComputeData {
    inv_view_projection_left_eye  : mat4x4f,
    inv_view_projection_right_eye : mat4x4f,

    left_eye_pos    : vec3f,
    render_height   : f32,
    right_eye_pos   : vec3f,
    render_width    : f32,

    time            : f32,
    dummy0          : f32,
    dummy1          : f32,
    dummy2          : f32,
};

@group(0) @binding(0) var left_eye_texture: texture_storage_2d<rgba8unorm,write>;
@group(0) @binding(1) var right_eye_texture: texture_storage_2d<rgba8unorm,write>;

@group(1) @binding(0) var<uniform> compute_data : sComputeData;

const MAX_DIST = 40.0;
const MIN_HIT_DIST = 0.0001;
const DERIVATIVE_STEP = 0.0001;

const ambientCoeff = 0.2;
const diffuseCoeff = 0.9;
const specularCoeff = 1.0;
const specularExponent = 64.0;
const lightPos = vec3f(0.0, 2.0, 1.0);

const fov = 45.0;
const up = vec3f(0.0, 1.0, 0.0);

fn sampleSdf(p : vec3f) -> Surface
{
    var res : Surface = opSmoothUnion(
        sdSphere(p, vec3f( 1.0 * sin(compute_data.time), 0.0, -2.0), 1.0, vec3f(1.0, 0.0, 0.0)),
        sdSphere(p, vec3f(-1.0, 0.0, -2.0), 1.0, vec3f(0.0, 1.0, 0.0)), 0.5);

    res = opSmoothUnion(res,
        sdPlane(p, vec3f(0.0, -0.4, 0.0), vec3f(0.0, 1.0, 0.0), 0.0, vec3f(0.0, 0.0, 1.0)), 0.4);

    return res;
}

fn estimateNormal(p : vec3f) -> vec3f
{
    return normalize(vec3f(
        sampleSdf(vec3f(p.x + DERIVATIVE_STEP, p.y, p.z)).distance - sampleSdf(vec3f(p.x - DERIVATIVE_STEP, p.y, p.z)).distance,
        sampleSdf(vec3f(p.x, p.y + DERIVATIVE_STEP, p.z)).distance - sampleSdf(vec3f(p.x, p.y - DERIVATIVE_STEP, p.z)).distance,
        sampleSdf(vec3f(p.x, p.y, p.z + DERIVATIVE_STEP)).distance - sampleSdf(vec3f(p.x, p.y, p.z - DERIVATIVE_STEP)).distance
    ));
}

fn blinnPhong(rayOrigin : vec3f, position : vec3f, lightPosition : vec3f, ambient : vec3f, diffuse : vec3f) -> vec3f
{
    let normal : vec3f = estimateNormal(position);
    let toEye : vec3f = normalize(rayOrigin - position);
    let toLight : vec3f = normalize(lightPosition - position);
    // let reflection : vec3f = reflect(-toLight, normal); // uncomment for Phong model
    let halfwayDir : vec3f = normalize(toLight + toEye);

    let ambientFactor : vec3f = ambient * ambientCoeff;
    let diffuseFactor : vec3f = diffuse * max(0.0, dot(normal, toLight));
    // let specularFactor : vec3f = diffuse * pow(max(0.0, dot(toEye, reflection)), specularExponent)
    //                     * specularCoeff; // uncomment for Phong model
    let specularFactor : vec3f = diffuse * pow(max(0.0, dot(normal, halfwayDir)), specularExponent)
                        * specularCoeff;

    return ambientFactor + diffuseFactor + specularFactor;
}

fn raymarch(rayOrigin : vec3f, rayDir : vec3f) -> vec3f
{
    let ambientColor = vec3f(0.2, 0.2, 0.2);
	let hitColor = vec3f(1.0, 1.0, 1.0);
	let missColor = vec3f(0.0, 0.0, 0.0);
    let lightOffset = vec3f(0.0, 0.0, 0.0);

	var depth = 0.0;
	var minDist = MAX_DIST;
	for (var i : i32 = 0; depth < MAX_DIST && i < 100; i++)
	{
		let pos = rayOrigin + rayDir * depth;
        let surface : Surface = sampleSdf(pos);
		if (surface.distance < MIN_HIT_DIST) {
			return blinnPhong(rayOrigin, pos, lightPos + lightOffset, ambientColor, surface.color);
		}
		depth += min(surface.distance, 0.5);
	}
    return missColor;
}

fn getRayDirection(inv_view_projection : mat4x4f, uv : vec2f) -> vec3f
{
	// convert coordinates from [0, 1] to [-1, 1]
	var screenCoord : vec4f = vec4f((uv - 0.5) * 2.0, 1.0, 1.0);

	var rayDir : vec4f = inv_view_projection * screenCoord;
    rayDir = rayDir / rayDir.w;

	return normalize(rayDir.xyz);
}

@compute @workgroup_size(16, 16, 1)
fn compute(@builtin(global_invocation_id) id: vec3<u32>) {

    let pixel_size = 1.0 / vec2f(compute_data.render_width, compute_data.render_height);
    var uv = vec2f(id.xy) * pixel_size;
    //uv.y = 1.0 - uv.y;
    let ray_dir_left = getRayDirection(compute_data.inv_view_projection_left_eye, uv);
    let ray_dir_right = getRayDirection(compute_data.inv_view_projection_right_eye, uv);

    textureStore(left_eye_texture, id.xy, vec4f(raymarch(compute_data.left_eye_pos, ray_dir_left), 1.0));
    textureStore(right_eye_texture, id.xy, vec4f(raymarch(compute_data.right_eye_pos, ray_dir_right), 1.0));
}
