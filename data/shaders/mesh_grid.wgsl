#include mesh_includes.wgsl

@group(0) @binding(0) var<storage, read> mesh_data : InstanceData;

#dynamic @group(1) @binding(0) var<uniform> camera_data : CameraData;

@group(2) @binding(1) var<uniform> albedo: vec4f;

const MAJOR_GRID_DIVISIONS : f32 = 10.0;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {

    let instance_data : RenderMeshData = mesh_data.data[in.instance_id];

    var out: VertexOutput;
    var world_position = instance_data.model * vec4f(in.position, 1.0);
    out.world_position = world_position.xyz;
    out.position = camera_data.view_projection * world_position;
    out.uv = in.uv; // forward to the fragment shader
    out.normal = in.normal;

    let div : f32 = max(2.0, round(MAJOR_GRID_DIVISIONS));

    // trick to reduce visual artifacts when far from the world origin
    let cameraCenteringOffset : vec3f = floor(camera_data.eye / div) * div;
    let offset : vec2f = (world_position.xyz - cameraCenteringOffset).xz;

    out.color = vec4f(offset.y, offset.x, world_position.z, world_position.x);

    return out;
}

struct FragmentOutput {
    @location(0) color: vec4f
}

const AXIS_LINE_WIDTH : f32 = 0.038;
const AXIS_DASH_SCALE : f32 = 1.0;
const MAJOR_LINE_WIDTH : f32 = 0.035;
const MINOR_LINE_WIDTH : f32 = 0.01;

// https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8#5ef5
// Based on this implementation: https://gist.github.com/bgolus/3a561077c86b5bfead0d6cc521097bae
fn pristine_grid(uv : vec4f, lineWidth : vec2f) -> vec4f
{
    let uvDDXY : vec4f = vec4f(dpdx(uv.xy), dpdy(uv.xy));
    let uvDeriv : vec2f = vec2f(length(uvDDXY.xz), length(uvDDXY.yw));

    let axisLineWidth : vec2f = vec2f(max(MAJOR_LINE_WIDTH, AXIS_LINE_WIDTH));
    let axisDrawWidth : vec2f = max(axisLineWidth, uvDeriv);
    let axisLineAA : vec2f = uvDeriv * 1.5;
    var axisLines2 : vec2f = smoothstep(axisDrawWidth + axisLineAA, axisDrawWidth - axisLineAA, abs(uv.zw * 2.0));
    axisLines2 *= saturate(axisLineWidth / axisDrawWidth);

    let div : f32 = max(2.0, round(MAJOR_GRID_DIVISIONS));
    let majorUVDeriv : vec2f = uvDeriv / div;
    let majorLineWidth : vec2f = vec2f(MAJOR_LINE_WIDTH / div);
    let majorDrawWidth : vec2f = clamp(majorLineWidth, majorUVDeriv, vec2f(0.5));
    let majorLineAA : vec2f = majorUVDeriv * 1.5;
    var majorGridUV : vec2f = 1.0 - abs(fract(uv.xy / div) * 2.0 - 1.0);
    let majorAxisOffset : vec2f = (1.0 - saturate(abs(uv.zw / div * 2.0))) * 2.0;
    majorGridUV += majorAxisOffset; // adjust UVs so center axis line is skipped
    var majorGrid2 : vec2f = smoothstep(majorDrawWidth + majorLineAA, majorDrawWidth - majorLineAA, majorGridUV);
    majorGrid2 *= saturate(majorLineWidth / majorDrawWidth);
    majorGrid2 = saturate(majorGrid2 - axisLines2); // hack
    majorGrid2 = mix(majorGrid2, majorLineWidth, saturate(majorUVDeriv * 2.0 - 1.0));

    let minorLineWidth : f32 = min(MINOR_LINE_WIDTH, MAJOR_LINE_WIDTH);
    let minorInvertLine : bool = minorLineWidth > 0.5;
    let minorTargetWidth : vec2f = vec2f(select(minorLineWidth, 1.0 - minorLineWidth, minorInvertLine));
    let minorDrawWidth : vec2f = clamp(minorTargetWidth, uvDeriv, vec2f(0.5));
    let minorLineAA : vec2f = uvDeriv * 1.5;
    var minorGridUV : vec2f = abs(fract(uv.xy) * 2.0 - 1.0);
    minorGridUV = select(1.0 - minorGridUV, minorGridUV, minorInvertLine);
    let minorMajorOffset : vec2f = (1.0 - saturate((1.0 - abs(fract(uv.zw / div) * 2.0 - 1.0)) * div)) * 2.0;
    minorGridUV += minorMajorOffset; // adjust UVs so major division lines are skipped
    var minorGrid2 : vec2f = smoothstep(minorDrawWidth + minorLineAA, minorDrawWidth - minorLineAA, minorGridUV);
    minorGrid2 *= saturate(minorTargetWidth / minorDrawWidth);
    minorGrid2 = saturate(minorGrid2 - axisLines2); // hack
    minorGrid2 = mix(minorGrid2, minorTargetWidth, saturate(uvDeriv * 2.0 - 1.0));
    minorGrid2 = select(minorGrid2, 1.0 - minorGrid2, minorInvertLine);
    minorGrid2 = select(vec2f(0.0), minorGrid2, abs(uv.zw) > vec2f(0.5));

    let minorGrid : f32 = mix(minorGrid2.x, 1.0, minorGrid2.y);
    let majorGrid : f32 = mix(majorGrid2.x, 1.0, majorGrid2.y);

    let axisDashUV : vec2f = abs(fract((uv.zw + axisLineWidth * 0.5) * AXIS_DASH_SCALE) * 2.0 - 1.0) - 0.5;
    let axisDashDeriv : vec2f = uvDeriv * AXIS_DASH_SCALE * 1.5;
    var axisDash : vec2f = smoothstep(-axisDashDeriv, axisDashDeriv, axisDashUV);
    axisDash = select(vec2f(1.0), axisDash, uv.zw < vec2f(0.0));

    let xAxisColor : vec4f = vec4f(1.0, 0.2, 0.2, 1.0);
    let yAxisColor : vec4f = vec4f(0.2, 1.0, 0.2, 1.0);
    let zAxisColor : vec4f = vec4f(0.2, 0.2, 1.0, 1.0);
    let xAxisDashColor : vec4f = vec4f(0.5, 0.2, 0.2, 1.0);
    let yAxisDashColor : vec4f = vec4f(0.2, 0.5, 0.2, 1.0);
    let zAxisDashColor : vec4f = vec4f(0.2, 0.2, 0.5, 1.0);
    let centerColor : vec4f = vec4f(1.0, 1.0, 1.0, 1.0);
    let majorLineColor : vec4f = vec4f(0.27, 0.27, 0.27, 1.0);
    let minorLineColor : vec4f = vec4f(0.44, 0.44, 0.44, 1.0);
    let baseColor : vec4f = vec4f(0.0, 0.0, 0.0, 0.0);

    var aAxisColor : vec4f = xAxisColor;
    var bAxisColor : vec4f = zAxisColor;
    let aAxisDashColor : vec4f = xAxisDashColor;
    let bAxisDashColor : vec4f = zAxisDashColor;

    aAxisColor = mix(aAxisDashColor, aAxisColor, axisDash.y);
    bAxisColor = mix(bAxisDashColor, bAxisColor, axisDash.x);
    aAxisColor = mix(aAxisColor, centerColor, axisLines2.y);

    let axisLines : vec4f = mix(bAxisColor * axisLines2.y, aAxisColor, axisLines2.x);

    var col : vec4f = mix(baseColor, minorLineColor, minorGrid *  minorLineColor.a);
    col = mix(col, majorLineColor, majorGrid * majorLineColor.a);
    col = col * (1.0 - axisLines.a) + axisLines;

    return col;
}

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {

    var dummy = camera_data.eye;
    var dummy2 = albedo;

    var out: FragmentOutput;

    out.color = pristine_grid(in.color, vec2f(MAJOR_LINE_WIDTH));

    out.color.a *= (1.0 - 2.0 * distance(vec2f(0.5), in.uv.xy));

    return out;
}