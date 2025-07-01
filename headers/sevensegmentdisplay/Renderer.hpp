#pragma once

#include "sevensegmentdisplay/Types.hpp"

#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

using std::vector, std::array, std::span, glm::vec2;

class Renderer
{
public:
    Renderer(const int& width, const int& height, const char* title);
    ~Renderer();
    void drawFrame(span<const Segment> segments, const vector<vector<vec2>>& bitSquares) const;
    [[nodiscard]] GLFWwindow* getWindow() const;

private:
    GLFWwindow* window;
    GLuint shaderProgram{};
    GLuint vao{};
    GLuint vbo{};
};

