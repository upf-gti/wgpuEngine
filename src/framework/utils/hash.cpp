#include "hash.h"

bool RenderPipelineKey::operator==(const RenderPipelineKey& other) const
{
    return (shader == other.shader
        && color_target.format == other.color_target.format
        && color_target.writeMask == other.color_target.writeMask
        && color_target.blend == other.color_target.blend
        && description.cull_mode == other.description.cull_mode
        && description.topology == other.description.topology);
}
