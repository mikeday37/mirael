#include "app_pch.hpp"

#include "app/utility.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/matrix_transform_2d.hpp"
#include "gtx/norm.hpp"
#include <cmath>

float PointDistanceToLineSegment(const glm::vec2 &P, const glm::vec2 &A, const glm::vec2 &B)
{
    auto AB = B - A;
    auto AP = P - A;

    float ls = glm::length2(AB);
    if (ls == 0) {
        return glm::distance(P, A);
    }

    auto t = glm::dot(AP, AB) / ls;
    if (t < 0.0f) {
        return glm::distance(P, A);
    } else if (t > 1.0f) {
        return glm::distance(P, B);
    }

    auto X = A + t * AB;
    return glm::distance(P, X);
}

glm::mat3 CalculateMappingTransform(glm::vec2 srcStart, glm::vec2 srcEnd, glm::vec2 dstStart, glm::vec2 dstEnd,
                                    bool mirrored)
{
    glm::mat3 transform = glm::mat3(1.0f);

    float srcLength = glm::length(srcEnd - srcStart), dstLength = glm::length(dstEnd - dstStart);
    if (srcLength == 0) {
        return transform; // just return identity matrix in the degenerate case
    }
    float scale = dstLength / srcLength;
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