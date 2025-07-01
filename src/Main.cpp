#include "sevensegmentdisplay/Main.hpp"
#include "sevensegmentdisplay/Renderer.hpp"

#include <fontconfig/fontconfig.h>
#include <chrono>

using namespace std;
using namespace glm;

uint8_t Main::bits = 0;
Segment segments[7];

constexpr double targetFPS = 60.0;
constexpr double targetFrameTime = 1.0 / targetFPS;

auto lastFrame = chrono::high_resolution_clock::now();

constexpr uint8_t digitToSegments[16] = {
    0b0111111, // 0
    0b0000110, // 1
    0b1011011, // 2
    0b1001111, // 3
    0b1100110, // 4
    0b1101101, // 5
    0b1111101, // 6
    0b0000111, // 7
    0b1111111, // 8
    0b1101111, // 9
    0b1110111, // A
    0b1111100, // b
    0b0111001, // C
    0b1011110, // d
    0b1111001, // E
    0b1110001 // F
};

constexpr void updateSegments(const uint8_t value)
{
    const uint8_t segBits = digitToSegments[value & 0xF];
    for (int i = 0; i < 7; ++i)
    {
        const bool isOn = (segBits >> i) & 1;
        segments[i].color = isOn ? vec3(1.0f, 0.0f, 0.0f) : vec3(0.2f);
    }
}

vector<vec2> createSegment(const float cx, const float cy, const float length, const float thickness, const float taper)
{
    return {
        {cx - length / 2 + taper, cy + thickness / 2},
        {cx + length / 2 - taper, cy + thickness / 2},
        {cx + length / 2, cy},
        {cx + length / 2 - taper, cy - thickness / 2},
        {cx - length / 2 + taper, cy - thickness / 2},
        {cx - length / 2, cy}
    };
}

vector<vec2> rotate90CCW(const vector<vec2>& points, const float cx, const float cy)
{
    vector<vec2> rotated;
    for (auto& p : points)
    {
        const float dx = p.x - cx;
        const float dy = p.y - cy;
        rotated.emplace_back(cx - dy, cy + dx);
    }
    return rotated;
}

vector<vec2> createSquare(const float cx, const float cy, const float size)
{
    const float half = size / 2.0f;
    return {
            {cx - half, cy - half},
            {cx + half, cy - half},
            {cx + half, cy + half},
            {cx - half, cy + half}
    };
}

int main()
{
    if (!FcInit()) {
        std::cerr << "Failed to initialize Fontconfig!" << std::endl;
        return -1;
    }

    constexpr float centerX = 300.0f;
    constexpr float centerY = 300.0f;

    constexpr float segmentLength = 120.0f;
    constexpr float thickness = 30.0f;
    constexpr float taper = 0.0f;

    constexpr float verticalX = segmentLength / 2 + thickness / 2;
    constexpr float upperVerticalY = centerY - segmentLength / 2 - thickness / 2;
    constexpr float lowerVerticalY = centerY + segmentLength / 2 + thickness / 2;

    segments[0].points = createSegment(centerX, centerY - segmentLength - thickness, segmentLength, thickness, taper);
    segments[3].points = createSegment(centerX, centerY + segmentLength + thickness, segmentLength, thickness, taper);
    segments[6].points = createSegment(centerX, centerY, segmentLength, thickness, taper);


    segments[1].points = rotate90CCW(
        createSegment(centerX + verticalX, upperVerticalY, segmentLength, thickness, taper),
        centerX + verticalX, upperVerticalY);
    segments[2].points = rotate90CCW(
        createSegment(centerX + verticalX, lowerVerticalY, segmentLength, thickness, taper),
        centerX + verticalX, lowerVerticalY);
    segments[4].points = rotate90CCW(
        createSegment(centerX - verticalX, lowerVerticalY, segmentLength, thickness, taper),
        centerX - verticalX, lowerVerticalY);
    segments[5].points = rotate90CCW(
        createSegment(centerX - verticalX, upperVerticalY, segmentLength, thickness, taper),
        centerX - verticalX, upperVerticalY);

    constexpr float squareSize = 50.0f;
    constexpr float gap = 10.0f;
    constexpr float baseY = centerY + segmentLength + thickness * 3.0f + 20.0f;

    constexpr float totalWidth = squareSize * 4 + gap * 3;
    constexpr float startX = centerX - totalWidth / 2 + squareSize / 2;

    vector<vector<vec2>> bitSquares(4);
    for (int i = 0; i < 4; ++i)
    {
        const float x = startX + i * (squareSize + gap);
        bitSquares[i] = createSquare(x, baseY, squareSize);
    }

    auto* renderer = new Renderer(600, 800, "Sieben-Segment-Display");
    GLFWwindow* window = renderer->getWindow();
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer->drawFrame(segments, bitSquares);

        glfwSwapBuffers(window);
        glfwPollEvents();
        updateSegments(Main::getBits());
    }

    delete renderer;


    return 0;
}

uint8_t Main::getBits()
{
    return bits;
}

void Main::setBits(const uint8_t& newBits)
{
    bits = newBits;
}
