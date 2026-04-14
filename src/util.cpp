#include "pch.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cmath>

namespace Mirael
{

glm::mat3 calculateMappingTransform(glm::vec2 srcStart, glm::vec2 srcEnd, glm::vec2 dstStart, glm::vec2 dstEnd, bool mirrored)
{
    glm::mat3 transform = glm::mat3(1.0f);

    float srcLength = glm::length(srcEnd - srcStart), dstLength = glm::length(dstEnd - dstStart);
    if (srcLength == 0) {
        return transform; // just return identity matrix in the degenerate case
    }
    float scale      = dstLength / srcLength;
    glm::vec2 srcDir = srcEnd - srcStart, dstDir = dstEnd - dstStart;
    float srcAngle = atan2(srcDir.y, srcDir.x), dstAngle = atan2(dstDir.y, dstDir.x);

    // apply mapping transformation steps in reverse order
    transform = glm::translate(transform, dstStart);
    transform = glm::rotate(transform, dstAngle);
    transform = glm::scale(transform, glm::vec2(scale, mirrored ? -scale : scale));
    transform = glm::rotate(transform, -srcAngle);
    transform = glm::translate(transform, -srcStart);

    return transform;
}

}; // namespace Mirael