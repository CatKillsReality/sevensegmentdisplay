#include "sevensegmentdisplay/Renderer.hpp"
#include "sevensegmentdisplay/Main.hpp"

#include <fstream>
#include <sstream>

using namespace std;
using namespace glm;

GLFWwindow* window;

GLuint shaderProgram;
GLuint vao, vbo;

GLuint compileShader(const GLenum type, const std::string& src)
{
    const GLuint shader = glCreateShader(type);
    const char* srcPtr = src.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << "\n";
        throw std::runtime_error("Shader compilation failed");
    }

    return shader;
}


vec2 toNDC(const float x, const float y)
{
    return {x / 300.0f - 1.0f, 1.0f - y / 400.0f};
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        uint8_t inputBits = Main::getBits();
        switch (key)
        {
        case GLFW_KEY_F4:
            inputBits ^= 1 << 0; // toggle bit 0
            break;
        case GLFW_KEY_F3:
            inputBits ^= 1 << 1; // toggle bit 1
            break;
        case GLFW_KEY_F2:
            inputBits ^= 1 << 2; // toggle bit 2
            break;
        case GLFW_KEY_F1:
            inputBits ^= 1 << 3; // toggle bit 3
            break;
        default:
            if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
            {
                inputBits = key - GLFW_KEY_0;
            }
            else if (key >= GLFW_KEY_A && key <= GLFW_KEY_F)
            {
                inputBits = 10 + (key - GLFW_KEY_A);
            }
            break;
        }
        Main::setBits(inputBits);
    }
}

Renderer::Renderer(const int& width, const int& height, const char* title)
{
    if (!glfwInit())
    {
        throw runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        throw runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, keyCallback);

    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        throw runtime_error("Failed to initialize GLAD");
    }

    glViewport(0, 0, width, height);

    constexpr auto vertexShaderSrc = R"glsl(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * vec4(aPos, 0.0, 1.0);
    }
    )glsl";

    constexpr auto fragmentShaderSrc = R"glsl(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 color;
    void main() {
        FragColor = vec4(color, 1.0);
    }
    )glsl";



    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);



    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(shaderProgram, sizeof(infoLog), nullptr, infoLog);
        cerr << "Shader Program linking failed: " << infoLog << "\n";
        throw runtime_error("Shader Program linking failed");
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(shaderProgram);

    constexpr float left = -1.0f;
    constexpr float right = 1.0f;
    constexpr float bottom = -1.0f;
    constexpr float top = 1.0f;
    constexpr float projection[16] = {
        2.0f / (right - left), 0, 0, 0,
        0, 2.0f / (top - bottom), 0, 0,
        0, 0, -1, 0,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0, 1
    };
    const GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);


    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, static_cast<void*>(nullptr));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Renderer::drawFrame(const span<const Segment> segments, const vector<vector<vec2>>& bitSquares) const
{


    glUseProgram(shaderProgram);
    glBindVertexArray(vao);

    // Draw segments
    for (const auto& segment : segments)
    {
        vector<float> vertices;
        for (const vec2& p : segment.points)
        {
            vec2 ndc = toNDC(p.x, p.y);
            vertices.push_back(ndc.x);
            vertices.push_back(ndc.y);
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                     vertices.data(),
                     GL_DYNAMIC_DRAW);

        GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc, segment.color.x, segment.color.y, segment.color.z);

        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size() / 2));
    }

    for (int i = 0; i < 4; ++i)
    {
        const int bitIndex = 3 - i;
        const uint8_t inputBits = Main::getBits();
        const bool isOn = (inputBits >> bitIndex) & 1;
        const vec3 color = isOn ? vec3(1.0f, 0.0f, 0.0f) : vec3(0.2f);

        vector<float> vertices;
        for (const vec2& p : bitSquares[i])
        {
            vec2 ndc = toNDC(p.x, p.y);
            vertices.push_back(ndc.x);
            vertices.push_back(ndc.y);
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                     vertices.data(),
                     GL_DYNAMIC_DRAW);

        const GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc, color.x, color.y, color.z);

        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size() / 2));
    }
}

GLFWwindow* Renderer::getWindow() const
{
    return window;
}



