#include "hash.h"

bool RenderPipelineKey::operator==(const RenderPipelineKey& other) const
{
    return (shader == other.shader
        && description.cull_mode == other.description.cull_mode
        && description.topology == other.description.topology
        && description.depth_read == other.description.depth_read
        && description.depth_write == other.description.depth_write
        && description.blending_enabled == other.description.blending_enabled
        && description.depth_compare == other.description.depth_compare
        && pipeline_layout == other.pipeline_layout);
}
