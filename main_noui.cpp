#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdio>
#include <sstream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

// 4K dimensions for preview and offline render
const int WIN_WIDTH = 3840;
const int WIN_HEIGHT = 2160;
const int OFF_WIDTH = 3840;
const int OFF_HEIGHT = 2160;

// Vertex shader (pass-through)
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
out vec2 fragUV;
void main()
{
    gl_Position = vec4(aPosition, 1.0);
    fragUV = aTexCoord;
}
)";

// Fragment shader (converted Rainbow Alien Noise with camera controls)
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec2 iResolution;
uniform float iTime;
uniform float iZoom;
uniform vec2 iCenter;

// 2D rotation matrix
mat2 rotate2D(float angle) {
    float c = cos(angle), s = sin(angle);
    return mat2(c, -s, s, c);
}

void main() {
    vec2 r = iResolution;
    float t = iTime * 0.25; // Slow down animation to 0.25x speed
    vec2 FC = gl_FragCoord.xy;

    vec4 o = vec4(0.0);
    float i, z = 0.0, d, s;

    // Adjust coordinates with zoom and center
    vec2 uv = (FC - 0.5 * r) / (r.y * iZoom) + iCenter;

    for (i = 0.0; i < 100.0; i++) {
        // Ray direction with zoom scaling
        vec3 p = z * normalize(vec3(uv * 2.0, 1.0));
        // Apply rotation to yz plane
        p.yz *= rotate2D(0.2);
        for (d = 5.0; d < 300.0; d += d) { // Increase iterations for more detail
            p += 0.8 * sin(p.yzx * d * 1.5 - t * 3.1415926535 / 10.0) / d; // Higher amplitude and frequency
        }
        s = 0.3 - abs(p.y);
        z += d = 0.5 * (0.01 + 0.5 * max(s, -s * 0.1));
        o += 0.5 * (cos(s / 0.07 + p.x + t * 3.1415926535 / 10.0 - vec4(0.0, 1.0, 2.0, 3.0) - 3.0) + 1.5) * exp(s * 9.0) / d*2; // Reduce color brightness
    }

    // Apply tanh saturation with darker tone
    o = tanh(o * o / 6e8);
    // Apply gamma correction for deeper colors
    o.rgb = pow(o.rgb, vec3(1.2));
    FragColor = vec4(o.rgb, 1.0);
}
)";

// Compile shader with error checking
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed (" 
                  << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") 
                  << "):\n" << infoLog << std::endl;
    }
    return shader;
}

// Link program with error checking
GLuint linkProgram(GLuint vertShader, GLuint fragShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Program linking failed:\n" << infoLog << std::endl;
    }
    return program;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create 4K preview window
    GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Shader 4K Preview", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

    // Compile and link shaders
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint shaderProgram = linkProgram(vertShader, fragShader);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    // Get uniform locations
    GLint iTimeLoc = glGetUniformLocation(shaderProgram, "iTime");
    GLint iResLoc = glGetUniformLocation(shaderProgram, "iResolution");
    GLint iZoomLoc = glGetUniformLocation(shaderProgram, "iZoom");
    GLint iCenterLoc = glGetUniformLocation(shaderProgram, "iCenter");

    // Setup full-screen quad
    float quadVertices[] = {
        // positions        // texCoords
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Preview timing
    double previewStart = glfwGetTime();
    std::cout << "Preview mode: Press R to start offline rendering, ESC to exit.\n";

    bool offlineRender = false;

    // Preview Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        float elapsed = static_cast<float>(glfwGetTime() - previewStart);
        glUniform1f(iTimeLoc, elapsed);
        glUniform2f(iResLoc, static_cast<float>(WIN_WIDTH), static_cast<float>(WIN_HEIGHT));
        // Set default camera parameters: adjust these as desired.
        glUniform1f(iZoomLoc, 2.0f);              // Back up the camera (zoom out)
        glUniform2f(iCenterLoc, 0.0f, 0.0f);        // Center offset at (0,0)
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glfwSwapBuffers(window);

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            offlineRender = true;
            break;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }
    }

    if (!offlineRender) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }

    // Offline Render Parameters
    int totalFrames;
    float desiredDuration, slowdownFactor;
    std::cout << "Enter total number of offline frames: ";
    std::cin >> totalFrames;
    std::cout << "Enter desired simulation duration (seconds): ";
    std::cin >> desiredDuration;
    std::cout << "Enter slowdown factor (1.0 = preview speed, <1 slows): ";
    std::cin >> slowdownFactor;

    // Generate unique output filename
    std::string baseName = "output";
    std::string extension = ".mp4";
    std::string outputFile = baseName + extension;
    int counter = 1;
    while (fs::exists(outputFile)) {
        std::ostringstream oss;
        oss << baseName << "_" << counter << extension;
        outputFile = oss.str();
        counter++;
    }

    // FFmpeg command
    std::string ffmpegCmd = "ffmpeg -y -f rawvideo -pixel_format rgb24 -video_size " +
                            std::to_string(OFF_WIDTH) + "x" + std::to_string(OFF_HEIGHT) +
                            " -framerate 60 -i - -c:v libx264 -pix_fmt yuv420p " + outputFile;

    // Offline Render Setup
    std::cout << "Starting 4K offline render...\n";
    FILE* ffmpegPipe = popen(ffmpegCmd.c_str(), "w");
    if (!ffmpegPipe) {
        std::cerr << "Failed to open ffmpeg pipe.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Setup off-screen framebuffer
    GLuint fbo, texColorBuffer;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texColorBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, OFF_WIDTH, OFF_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer incomplete!\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Offline render loop
    std::vector<unsigned char> frameBuffer(OFF_WIDTH * OFF_HEIGHT * 3);
    for (int frame = 0; frame < totalFrames; ++frame) {
        float simulatedTime = (frame / static_cast<float>(totalFrames - 1)) * desiredDuration * slowdownFactor;
        
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, OFF_WIDTH, OFF_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glUniform1f(iTimeLoc, simulatedTime);
        glUniform2f(iResLoc, static_cast<float>(OFF_WIDTH), static_cast<float>(OFF_HEIGHT));
        // Use the same camera settings for offline render.
        glUniform1f(iZoomLoc, 2.0f);
        glUniform2f(iCenterLoc, 0.0f, 0.0f);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glFinish();

        glReadPixels(0, 0, OFF_WIDTH, OFF_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());
        if (fwrite(frameBuffer.data(), 1, frameBuffer.size(), ffmpegPipe) != frameBuffer.size()) {
            std::cerr << "Error writing frame " << frame << " to ffmpeg.\n";
        }
        std::cout << "Rendered frame " << frame + 1 << " of " << totalFrames << "\n";
    }

    // Cleanup
    pclose(ffmpegPipe);
    std::cout << "Offline render complete. Saved as " << outputFile << "\n";
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texColorBuffer);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
