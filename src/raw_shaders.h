#pragma once

namespace RAW_SHADERS {
    const char simple_shaders[] = R"(

struct VertexInput {
    @location(0) position: vec2f,
    @location(1) uv: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(in.position, 0.0, 1.0);
    out.uv = in.uv; // forward to the fragment shader
    return out;
}

@group(0) @binding(0) var render_texture: texture_2d<f32>;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    var texture_size = textureDimensions(render_texture);
    var uv_flip = in.uv;
    uv_flip.y = 1.0 - uv_flip.y;
    let color = textureLoad(render_texture, vec2u(uv_flip * vec2f(texture_size)) , 0);
    return color;
}

)";

    const char mirror_shaders[] = R"(

struct VertexInput {
    @location(0) position: vec2f,
    @location(1) uv: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(in.position, 0.0, 1.0);
    out.uv = in.uv; // forward to the fragment shader
    return out;
}

@group(0) @binding(0) var leftEyeTexture: texture_2d<f32>;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    var texture_size = textureDimensions(leftEyeTexture);
    var uv_flip = in.uv;
    uv_flip.y = 1.0 - uv_flip.y;
    let color = textureLoad(leftEyeTexture, vec2u(uv_flip * vec2f(texture_size)) , 0);
    return color;
}

)";

    const char compute_shader[] = R"(

struct sComputeData {
    inv_view_projection_left_eye : mat4x4f,
    inv_view_projection_right_eye : mat4x4f,

    left_eye_pos : vec3f,
    render_height : f32,
    right_eye_pos : vec3f,
    render_width : f32,
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

fn opSmoothUnion( d1 : vec4f, d2 : vec4f, k : f32 ) -> vec4f
{
    let h : f32 = clamp( 0.5 + 0.5*(d2.x-d1.x)/k, 0.0, 1.0 );
    return vec4f(mix( d2.x, d1.x, h ) - k*h*(1.0-h), mix(d2.yzw, d1.yzw, h));
}

fn sdPlane( p : vec3f, c : vec3f, n : vec3f, h : f32, color : vec3f ) -> vec4f
{
    // n must be normalized
    return vec4(dot(p - c, n.xyz) + h, color);
}

fn sdSphere( p : vec3f, c : vec3f, s : f32, color : vec3f) -> vec4f
{
    return vec4f(length(c-p)-s, color);
}

fn sdf(p : vec3f) -> vec4f
{
    var res : vec4f = opSmoothUnion(
        sdSphere(p, vec3f( 1.0, 0.0, -2.0), 1.0, vec3f(1.0, 0.0, 0.0)),
        sdSphere(p, vec3f(-1.0, 0.0, -2.0), 1.0, vec3f(0.0, 1.0, 0.0)), 0.5);

    res = opSmoothUnion(res,
        sdPlane(p, vec3f(0.0, -0.4, 0.0), vec3f(0.0, 1.0, 0.0), 0.0, vec3f(0.0, 0.0, 1.0)), 0.4);

    return res;
}

fn estimateNormal(p : vec3f) -> vec3f
{
    return normalize(vec3f(
        sdf(vec3f(p.x + DERIVATIVE_STEP, p.y, p.z)).x - sdf(vec3f(p.x - DERIVATIVE_STEP, p.y, p.z)).x,
        sdf(vec3f(p.x, p.y + DERIVATIVE_STEP, p.z)).x - sdf(vec3f(p.x, p.y - DERIVATIVE_STEP, p.z)).x,
        sdf(vec3f(p.x, p.y, p.z  + DERIVATIVE_STEP)).x - sdf(vec3f(p.x, p.y, p.z - DERIVATIVE_STEP)).x
    ));
}

fn blinnPhong(position : vec3f, lightPosition : vec3f, ambient : vec3f, diffuse : vec3f) -> vec3f
{
    let normal : vec3f = estimateNormal(position);
    let toEye : vec3f = normalize(vec3(0.0, 0.0, 0.0) - position);
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
    let ambientColor = vec3f(0.4, 0.4, 0.4);
	let hitColor = vec3f(1.0, 1.0, 1.0);
	let missColor = vec3f(0.0, 0.0, 0.0);
    let lightOffset = vec3f(4.0, 0.0, 0.0);

	var depth = 0.0;
	var minDist = MAX_DIST;
	for (var i : i32 = 0; depth < MAX_DIST && i < 100; i++)
	{
		let pos = rayOrigin + rayDir * depth;
		let dist : vec4f = sdf(pos);
		minDist = min(minDist, dist.x);
		if (minDist < MIN_HIT_DIST) {
            let lightningColor : vec3f = blinnPhong(pos, lightPos + lightOffset, ambientColor, dist.yzw);
			return lightningColor;
		}
		depth += dist.x;
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

)";

};