#include "sevensegmentdisplay/Main.hpp"
#include "sevensegmentdisplay/Renderer.hpp"

#include <fontconfig/fontconfig.h>
#include <chrono>
#include <thread>

using namespace std;
using namespace glm;

uint8_t Main::bits = 0;
float Main::frameNum = 0;

constexpr double targetFPS = 60.0;
constexpr double targetFrameTime = 1.0 / targetFPS;

auto previousTime = std::chrono::high_resolution_clock::now();

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

array<Segment, 7> calculateSegments(const vec2 screenSize, const uint8_t segmentValue)
{
    array<Segment, 7> segments;
    const float centerX = screenSize.x / 2.0f;
    const float yOffset = screenSize.y * -0.1;
    const float centerY = screenSize.y / 2.0f + yOffset;

    const float segmentLength = screenSize.y * 0.25f;
    const float thickness = screenSize.y * 0.06f;


    const float verticalX = segmentLength / 2 + thickness / 2;
    const float upperVerticalY = centerY - segmentLength / 2 - thickness / 2;
    const float lowerVerticalY = centerY + segmentLength / 2 + thickness / 2;

    segments[0].points = createSegment(centerX, centerY - segmentLength - thickness, segmentLength, thickness, 0);
    segments[3].points = createSegment(centerX, centerY + segmentLength + thickness, segmentLength, thickness, 0);
    segments[6].points = createSegment(centerX, centerY, segmentLength, thickness, 0);


    segments[1].points = rotate90CCW(
        createSegment(centerX + verticalX, upperVerticalY, segmentLength, thickness, 0),
        centerX + verticalX, upperVerticalY);
    segments[2].points = rotate90CCW(
        createSegment(centerX + verticalX, lowerVerticalY, segmentLength, thickness, 0),
        centerX + verticalX, lowerVerticalY);
    segments[4].points = rotate90CCW(
        createSegment(centerX - verticalX, lowerVerticalY, segmentLength, thickness, 0),
        centerX - verticalX, lowerVerticalY);
    segments[5].points = rotate90CCW(
        createSegment(centerX - verticalX, upperVerticalY, segmentLength, thickness, 0),
        centerX - verticalX, upperVerticalY);

    const uint8_t segBits = digitToSegments[segmentValue & 0xF];
    for (int i = 0; i < 7; ++i)
    {
        const bool isOn = (segBits >> i) & 1;
        segments[i].color = isOn ? vec3(1.0f, 0.0f, 0.0f) : vec3(1.0f);
    }
    return segments;
}

auto calculateBitIndicators(const vec2 screenSize)
{

    array<Segment, 7> segments;
    const float centerX = screenSize.x / 2.0f;
    const float yOffset = screenSize.y * -0.1;
    const float centerY = screenSize.y / 2.0f + yOffset;

    const float segmentLength = screenSize.y * 0.25f;
    const float thickness = screenSize.y * 0.06f;

    const float squareSize = screenSize.y * 0.06f;
    const float gap = screenSize.x * 0.01f;

    const float baseY = centerY + segmentLength + thickness * 3.0f + 20.0f;

    const float totalWidth = squareSize * 4 + gap * 3;
    const float startX = centerX - totalWidth / 2 + squareSize / 2;

    vector<vector<vec2>>  bitIndicators(4);
    for (int i = 0; i < 4; ++i)
    {
        const float x = startX + i * (squareSize + gap);
        bitIndicators[i] = createSquare(x, baseY, squareSize);
    }
    return bitIndicators;
}

int main()
{
    if (!FcInit()) {
        std::cerr << "Failed to initialize Fontconfig!" << std::endl;
        return -1;
    }

    const auto* renderer = new Renderer(400, 600, "Sieben-Segment-Display");
    GLFWwindow* window = renderer->getWindow();
    while (!glfwWindowShouldClose(window))
    {

        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = currentTime - previousTime;
        if (elapsed.count() < targetFrameTime)
        {
            std::this_thread::sleep_for(
                std::chrono::duration<double>(targetFrameTime - elapsed.count()));
        }
        previousTime = std::chrono::high_resolution_clock::now();


        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        renderer->drawFrame(calculateSegments(renderer->getScreenSize(), Main::getBits()), calculateBitIndicators(renderer->getScreenSize()));

        glfwSwapBuffers(window);
        glfwPollEvents();

        (*Main::getFramePtr())++;
    }

    delete renderer;


    return 0;
}

uint8_t Main::getBits()
{
    return bits;
}

float Main::getFrame()
{
    return frameNum;
}

float* Main::getFramePtr()
{
    return &frameNum;
}

void Main::setBits(const uint8_t& newBits)
{
    bits = newBits;
}
