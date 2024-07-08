// Copyright 2016 Activision Publishing, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

@group(0) @binding(0) var input_cubemap_texture: texture_cube<f32>;
@group(0) @binding(1) var output_cubemap_texture: texture_storage_2d_array<rgba32float, write>;
@group(0) @binding(2) var texture_sampler : sampler;

fn get_dir_0(u : f32, v : f32) -> vec3f 
{
	var dir_out : vec3f;
	dir_out[0] = 1.0;
	dir_out[1] = v;
	dir_out[2] = -u;
	return dir_out;
}

fn get_dir_1(u : f32, v : f32) -> vec3f 
{
	var dir_out : vec3f;
	dir_out[0] = -1.0;
	dir_out[1] = v;
	dir_out[2] = u;
	return dir_out;
}

fn get_dir_2(u : f32, v : f32) -> vec3f 
{
	var dir_out : vec3f;
	dir_out[0] = u;
	dir_out[1] = 1.0;
	dir_out[2] = -v;
	return dir_out;
}

fn get_dir_3(u : f32, v : f32) -> vec3f 
{
	var dir_out : vec3f;
	dir_out[0] = u;
	dir_out[1] = -1.0;
	dir_out[2] = v;
	return dir_out;
}

fn get_dir_4(u : f32, v : f32) -> vec3f 
{
	var dir_out : vec3f;
	dir_out[0] = u;
	dir_out[1] = v;
	dir_out[2] = 1.0;
	return dir_out;
}

fn get_dir_5(u : f32, v : f32) -> vec3f 
{
	var dir_out : vec3f;
	dir_out[0] = -u;
	dir_out[1] = v;
	dir_out[2] = -1.0;
	return dir_out;
}

fn calcWeight(u : f32, v : f32) -> f32
{
	let val : f32 = u * u + v * v + 1.0;
	return val * sqrt(val);
}

// Based on: https://github.com/godotengine/godot/blob/master/servers/rendering/renderer_rd/shaders/effects/cubemap_downsampler.glsl
@compute @workgroup_size(8, 8, 1)

fn compute(@builtin(global_invocation_id) id: vec3<u32>) {

	let dim : vec2u = textureDimensions(output_cubemap_texture).xy;
	let face_size : u32 = dim.x;

	if (id.x < face_size && id.y < face_size) {
		let inv_face_size : f32 = 1.0 / f32(face_size);

		let u0 : f32 = (f32(id.x) * 2.0 + 1.0 - 0.75) * inv_face_size - 1.0;
		let u1 : f32 = (f32(id.x) * 2.0 + 1.0 + 0.75) * inv_face_size - 1.0;

		let v0 = (f32(id.y) * 2.0 + 1.0 - 0.75) * -inv_face_size + 1.0;
		let v1 = (f32(id.y) * 2.0 + 1.0 + 0.75) * -inv_face_size + 1.0;

		var weights : array<f32, 4>;
		weights[0] = calcWeight(u0, v0);
		weights[1] = calcWeight(u1, v0);
		weights[2] = calcWeight(u0, v1);
		weights[3] = calcWeight(u1, v1);

		let wsum : f32 = 0.5 / (weights[0] + weights[1] + weights[2] + weights[3]);
		for (var i : u32 = 0; i < 4; i++) {
			weights[i] = weights[i] * wsum + 0.125;
		}

		var dir : vec3f;
		var color : vec4f;
		switch (id.z) {
			case 0: {
				dir = get_dir_0(u0, v0);
				color = textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[0];

				dir = get_dir_0(u1, v0);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[1];

				dir = get_dir_0(u0, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[2];

				dir = get_dir_0(u1, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[3];
				break;
			}
			case 1: {
				dir = get_dir_1(u0, v0);
				color = textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[0];

				dir = get_dir_1(u1, v0);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[1];

				dir = get_dir_1(u0, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[2];

				dir = get_dir_1(u1, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[3];
				break;
			}
			case 2: {
				dir = get_dir_2(u0, v0);
				color = textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[0];

				dir = get_dir_2(u1, v0);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[1];

				dir = get_dir_2(u0, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[2];

				dir = get_dir_2(u1, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[3];
				break;
			}
			case 3: {
				dir = get_dir_3(u0, v0);
				color = textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[0];

				dir = get_dir_3(u1, v0);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[1];

				dir = get_dir_3(u0, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[2];

				dir = get_dir_3(u1, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[3];
				break;
			}
			case 4: {
				dir = get_dir_4(u0, v0);
				color = textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[0];

				dir = get_dir_4(u1, v0);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[1];

				dir = get_dir_4(u0, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[2];

				dir = get_dir_4(u1, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[3];
				break;
			}
			default: {
				dir = get_dir_5(u0, v0);
				color = textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[0];

				dir = get_dir_5(u1, v0);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[1];

				dir = get_dir_5(u0, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[2];

				dir = get_dir_5(u1, v1);
				color += textureSampleLevel(input_cubemap_texture, texture_sampler, normalize(dir), 0.0) * weights[3];
				break;
			}
		}

		textureStore(output_cubemap_texture, id.xy, id.z, color);
	}
}