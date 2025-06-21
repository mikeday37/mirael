#include "app_pch.hpp"

#include "app/utility.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/norm.hpp"

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
