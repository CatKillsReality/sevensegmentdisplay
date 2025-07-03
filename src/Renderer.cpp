#include "sevensegmentdisplay/Renderer.hpp"
#include "sevensegmentdisplay/Main.hpp"

#include <fstream>

using namespace std;
using namespace glm;


mat4 projection;
GLFWwindow *window;

GLuint shaderProgram;
GLuint vao, vbo;
GLint projectionLoc;

GLuint compileShader(const GLenum type, const std::string &src) {
    const GLuint shader = glCreateShader(type);
    const char *srcPtr = src.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << "\n";
        throw std::runtime_error("Shader compilation failed");
    }

    return shader;
}

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        uint8_t inputBits = Main::getBits();
        switch (key) {
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
                if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
                    inputBits = key - GLFW_KEY_0;
                } else if (key >= GLFW_KEY_A && key <= GLFW_KEY_F) {
                    inputBits = 10 + (key - GLFW_KEY_A);
                }
                break;
        }
        Main::setBits(inputBits);
    }
}

void resizeCallback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);

    if (auto *renderer = static_cast<Renderer *>(glfwGetWindowUserPointer(window))) {
        renderer->setScreenSize({width, height});
    } else throw runtime_error("Failed to resize window due to invalid Renderer pointer");
    projection = ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));
}

Renderer::Renderer(const int &width, const int &height, const char *title) {
    screenSize = {width, height};
    projection = ortho(0.0f, screenSize.x, screenSize.y, 0.0f);

    if (!glfwInit()) {
        throw runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);


    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw runtime_error("Failed to create GLFW window");
    }
    glfwSetWindowUserPointer(window, this);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, resizeCallback);

    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw runtime_error("Failed to initialize GLAD");
    }

    glViewport(0, 0, width, height);

    constexpr auto vertexShaderSrc = R"glsl(
    #version 330 core
    layout(location = 0) in vec2 aPos;      // vertex position input
    layout(location = 1) in vec2 aUV;       // UV input
    layout(location = 2) in vec3 aColor;    // vertex color input


    out vec2 fragUV;
    out vec3 fragColor;                     // pass to fragment shader
    uniform mat4 projection;                // uniform projection matrix

    void main()
    {
        gl_Position = projection * vec4(aPos, 0.0, 1.0);
        fragUV = aUV;
        fragColor = aColor;
    }
    )glsl";

    constexpr auto fragmentShaderSrc = R"glsl(
    #version 330 core
uniform vec2 resolution;
in vec2 fragUV;
in vec3 fragColor;
out vec4 FragColor;

uniform float time;

float hash(vec2 p) {
    return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x))));
}

float noise(vec2 x) {
    vec2 i = floor(x);
    vec2 f = fract(x);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 p) {
    float value = 0.0;
    float freq = 1.0;
    float amp = 0.5;
    for (int i = 0; i < 14; ++i) {
        value += amp * noise((p - vec2(1.0)) * freq);
        freq *= 1.9;
        amp *= 0.6;
    }
    return value;
}

float pattern(vec2 p) {
    vec2 aPos = vec2(sin(time/30 * 0.005), sin((time/30) * 0.01)) * 6.0;
    float a = fbm(p * vec2(3.0) + aPos);

    vec2 bPos = vec2(sin((time/30) * 0.01), sin((time/30) * 0.01)) * 1.0;
    float b = fbm((p + a) * vec2(0.6) + bPos);

    vec2 cPos = vec2(-0.6, -0.5) + vec2(sin(-(time/30) * 0.001), sin((time/30) * 0.01)) * 2.0;
    float c = fbm((p + b) * vec2(2.6) + cPos);

    return c;
}

vec3 red_palette(float t) {
    vec3 a = vec3(0.55, 0.0, 0.0);  // brighter dark red base
    vec3 b = vec3(0.2, 0.0, 0.0);  // smaller amplitude for less dark dips
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.0, 0.0, 0.0);
    return a + b * cos(6.28318 * (c * t + d));
}

vec3 gray_palette(float t) {
    vec3 a = vec3(0.1, 0.1, 0.1);
    vec3 b = vec3(0.05, 0.05, 0.05);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.0, 0.0, 0.0);
    return a + b * cos(6.28318 * (c * t + d));
}

vec3 white_palette(float t) {
    vec3 a = vec3(0.9, 0.9, 0.9);
    vec3 b = vec3(0.1, 0.1, 0.1);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.0, 0.0, 0.0);
    return a + b * cos(6.28318 * (c * t + d));
}

void main() {
    vec2 uv = fragUV * 2.0 - 1.0;
    float aspect = resolution.x / resolution.y;
    uv.x *= aspect;
    float val = pow(pattern(uv), 2.0);

    if (fragColor.r == 1.0 && fragColor.g == 0.0 && fragColor.b == 0.0) {
        FragColor = vec4(red_palette(val), 1.0);
    } else if (fragColor.r == 0.2 && fragColor.g == 0.2 && fragColor.b == 0.2) {
        FragColor = vec4(gray_palette(val), 1.0);
    } else if (fragColor.r == 1.0 && fragColor.g == 1.0 && fragColor.b == 1.0) {
        FragColor = vec4(white_palette(val), 1.0);
    } else {
        FragColor = vec4(fragColor, 1.0);
    }
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

    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(shaderProgram, sizeof(infoLog), nullptr, infoLog);
        cerr << "Shader Program linking failed: " << infoLog << "\n";
        throw runtime_error("Shader Program linking failed");
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(shaderProgram);

    projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));
    glUniform2f(glGetUniformLocation(shaderProgram, "resolution"), screenSize.x, screenSize.y);
    glUniform1f(glGetUniformLocation(shaderProgram, "time"), Main::getFrame());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    constexpr GLsizei stride = 7 * sizeof(float);

    // position (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

    // uv (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // color (location 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(4 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Renderer::~Renderer() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Renderer::drawFrame(const array<Segment, 7> &segments, const vector<vector<vec2> > &bitSquares) const {
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);


    glUniform1f(glGetUniformLocation(shaderProgram, "time"), Main::getFrame());
    glUniform2f(glGetUniformLocation(shaderProgram, "resolution"), screenSize.x, screenSize.y);

    // Draw background
    const vec3 bgColor = vec3(0.2f);
    const vector<float> bgVertices =  {
        0, 0, 0, 0, bgColor.r, bgColor.g, bgColor.b,
        screenSize.x, 0, 1, 0, bgColor.r, bgColor.g, bgColor.b,
        screenSize.x, screenSize.y, 1, 1, bgColor.r, bgColor.g, bgColor.b,
        0, screenSize.y, 0, 1, bgColor.r, bgColor.g, bgColor.b
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(bgVertices.size() * sizeof(float)),
                 bgVertices.data(),
                 GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(bgVertices.size() / 7));

    // Draw segments
    for (const auto &[points, color]: segments) {
        vector<float> vertices;
        // For each point, add x,y then r,g,b
        for (const vec2 &p: points) {
            vec2 uv = vec2(p.x / screenSize.x, p.y / screenSize.y);

            vertices.push_back(p.x);
            vertices.push_back(p.y);
            vertices.push_back(uv.x);
            vertices.push_back(uv.y);
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                     vertices.data(),
                     GL_DYNAMIC_DRAW);

        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size() / 7));
    }

    // Draw bit Indicators
    for (int i = 0; i < 4; ++i) {
        const int bitIndex = 3 - i;
        const uint8_t inputBits = Main::getBits();
        const bool isOn = (inputBits >> bitIndex) & 1;
        const vec3 color = isOn ? vec3(1.0f, 0.0f, 0.0f) : vec3(1.0f);

        vector<float> vertices;
        // Add position and color per vertex
        for (const vec2 &p: bitSquares[i]) {
            vec2 uv = vec2(p.x / screenSize.x, p.y / screenSize.y);

            vertices.push_back(p.x);
            vertices.push_back(p.y);
            vertices.push_back(uv.x);
            vertices.push_back(uv.y);
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                     vertices.data(),
                     GL_DYNAMIC_DRAW);

        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size() / 7));
    }

    glBindVertexArray(0);
}


GLFWwindow *Renderer::getWindow() const {
    return window;
}

vec2 Renderer::getScreenSize() const {
    return screenSize;
}

void Renderer::setScreenSize(const vec2 newScreenSize) {
    screenSize.x = newScreenSize.x;
    screenSize.y = newScreenSize.y;
}
