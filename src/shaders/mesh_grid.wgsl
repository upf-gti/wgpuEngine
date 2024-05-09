#include mesh_includes.wgsl

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

@group(2) @binding(1) var<uniform> albedo: vec4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {

    let instance_data : RenderMeshData = mesh_data.data[in.instance_id];

    var out: VertexOutput;
    var world_position = instance_data.model * vec4f(in.position, 1.0);
    out.world_position = world_position.xyz;
    out.position = camera_data.view_projection * world_position;
    out.uv = in.uv; // forward to the fragment shader
    out.color = vec4(in.color, 1.0) * albedo;
    out.normal = in.normal;
    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f
}

// https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8#5ef5
fn pristine_grid(uv : vec2f, lineWidth : vec2f) -> f32
{
    let ddx : vec2f = dpdx(uv);
    let ddy : vec2f = dpdy(uv);
    let uvDeriv : vec2f = vec2f(length(vec2f(ddx.x, ddy.x)), length(vec2(ddx.y, ddy.y)));
    let invertLine : vec2<bool> = vec2<bool>(lineWidth.x > 0.5, lineWidth.y > 0.5);
    let targetWidth : vec2f = vec2f(
      select(lineWidth.x, 1.0 - lineWidth.x, invertLine.x),
      select(lineWidth.y, 1.0 - lineWidth.y, invertLine.y));

    let drawWidth : vec2f = clamp(targetWidth, uvDeriv, vec2f(0.5));
    let lineAA : vec2f = uvDeriv * 1.5;
    var gridUV : vec2f = abs(fract(uv) * 2.0 - 1.0);
    gridUV.x = select(1.0 - gridUV.x, gridUV.x, invertLine.x);
    gridUV.y = select(1.0 - gridUV.y, gridUV.y, invertLine.y);
    var grid2 : vec2f = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);

    grid2 *= clamp(targetWidth / drawWidth, vec2f(0.0), vec2f(1.0));
    grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, vec2f(0.0), vec2f(1.0)));
    grid2.x = select(grid2.x, 1.0 - grid2.x, invertLine.x);
    grid2.y = select(grid2.y, 1.0 - grid2.y, invertLine.y);

    return mix(grid2.x, 1.0, grid2.y);
}

const GRID_AREA_SIZE : f32 = 10.0;
const GRID_QUAD_SIZE :f32 = 0.5;
const LINE_WIDTH : f32 = 0.02;

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {

    var dummy = camera_data.eye;

    let wrapped_uvs : vec2f = (in.uv * GRID_AREA_SIZE / GRID_QUAD_SIZE);
    let line_width_proportion : f32 = GRID_QUAD_SIZE * LINE_WIDTH;
    let one_minus_line_width : f32 = 1.0 - line_width_proportion;
    let zero_plus_line_width : f32 = 0.0 + line_width_proportion;

    var out: FragmentOutput;

    out.color = vec4f(0.27, 0.27, 0.27, 1.0) *
                pristine_grid(wrapped_uvs, vec2f(LINE_WIDTH));

    out.color.a *= (1.0 - 2.0 * distance(vec2f(0.5), in.uv.xy));

    return out;
}