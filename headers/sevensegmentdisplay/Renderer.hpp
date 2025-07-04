#pragma once

#include "sevensegmentdisplay/Types.hpp"

#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;
using std::vector, std::array, std::span;

class Renderer
{
public:
    Renderer(const int& width, const int& height, const char* title);
    ~Renderer();
    void drawFrame(const array<Segment, 7>& segments, const vector<vector<vec2>>& bitSquares) const;
    [[nodiscard]] GLFWwindow* getWindow() const;
    [[nodiscard]] static vec2 getScreenSize();
    static void setScreenSize(vec2 newScreenSize);

private:
    GLFWwindow* window;
    mat4 projection{};
    static vec2 screenSize;
    GLuint shaderProgram{};
    GLuint vao{};
    GLuint vbo{};
};

