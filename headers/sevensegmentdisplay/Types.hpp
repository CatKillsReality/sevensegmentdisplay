#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

using std::vector;
using glm::vec2;
using glm::vec3;

struct Segment {
    vector<vec2> points;
    vec3 color = vec3(1.0f, 1.0f, 1.0f);
};
