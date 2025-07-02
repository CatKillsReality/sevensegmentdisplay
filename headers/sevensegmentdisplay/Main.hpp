#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
using glm::vec2, glm::vec3, std::vector;

class Main
{
public:
    static void setBits(const uint8_t& bits);
    static uint8_t getBits();
    static float getFrame();
    static float* getFramePtr();

private:
    static uint8_t bits;
    static float frameNum;
};