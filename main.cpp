#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

// Default window dimensions (4K)
int WIN_WIDTH = 3840;
int WIN_HEIGHT = 2160;
int OFF_WIDTH = 3840;
int OFF_HEIGHT = 2160;

// Vertex shader
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

// Fallback fragment shader (solid red for debugging)
const char* fallbackFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color
}
)";

// Compile shader with error checking
bool compileShader(GLenum type, const char* source, GLuint& shader, std::string& error) {
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        error = "Shader compilation failed (" +
                std::string(type == GL_VERTEX_SHADER ? "vertex" : "fragment") +
                "):\n" + infoLog;
        glDeleteShader(shader);
        return false;
    }
    return true;
}

// Link program with error checking
bool linkProgram(GLuint vertShader, GLuint fragShader, GLuint& program, std::string& error) {
    program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        error = "Program linking failed:\n" + std::string(infoLog);
        glDeleteProgram(program);
        return false;
    }
    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        error = "Program validation failed:\n" + std::string(infoLog);
        glDeleteProgram(program);
        return false;
    }
    return true;
}

// Load shader from file
std::string loadShaderFile(const std::string& filepath, std::string& error) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        error = "Failed to open shader file: " + filepath;
        std::cerr << error << "\n";
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    if (content.empty()) {
        error = "Shader file is empty: " + filepath;
        std::cerr << error << "\n";
        return "";
    }
    std::cerr << "Loaded shader: " << filepath << ", size: " << content.size() << " bytes\n";
    return content;
}

// Load all shader files from directory
std::vector<std::string> loadShaderFiles(const std::string& directory, std::string& error) {
    std::vector<std::string> shaderFiles;
    try {
        if (!fs::exists(directory)) {
            error = "Shader directory does not exist: " + directory;
            std::cerr << error << "\n";
            return shaderFiles;
        }
        for (const auto& entry : fs::directory_iterator(directory)) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (ext == ".txt") {
                shaderFiles.push_back(entry.path().string());
                std::cerr << "Found .txt shader file: " << entry.path().string() << "\n";
            } else {
                std::cerr << "Skipped file (non-.txt extension): " << entry.path().string() << "\n";
            }
        }
        if (shaderFiles.empty()) {
            error = "No .txt shader files found in directory: " + directory;
            std::cerr << error << "\n";
        }
    } catch (const std::exception& e) {
        error = "Error reading shader directory: " + std::string(e.what());
        std::cerr << error << "\n";
    }
    return shaderFiles;
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

    // Create window
    GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Shader Preview", nullptr, nullptr);
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

    // Log OpenGL version
    std::cerr << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
    std::cerr << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    std::cerr << "ImGui version: " << ImGui::GetVersion() << "\n";

    // Load shaders
    std::string shaderDir = "shaders";
    std::string loadError;
    std::vector<std::string> shaderFiles = loadShaderFiles(shaderDir, loadError);
    if (shaderFiles.empty()) {
        std::cerr << loadError << ". Using fallback shader.\n";
        shaderFiles.push_back("fallback");
    }
    // Store shader names in a stable vector
    std::vector<std::string> shaderNames;
    for (const auto& file : shaderFiles) {
        std::string name = (file != "fallback") ? fs::path(file).filename().string() : "Fallback Shader";
        shaderNames.push_back(name);
        std::cerr << "Stored shader name: " << name << "\n";
    }
    // Create c_str pointers after all names are added
    std::vector<const char*> shaderNamesCStr;
    for (const auto& name : shaderNames) {
        shaderNamesCStr.push_back(name.c_str());
        std::cerr << "Added c_str to dropdown: " << name << "\n";
    }
    int currentShaderIndex = 0;

    // Compile initial shaders
    std::string shaderError;
    GLuint vertShader, fragShader, shaderProgram = 0;
    if (!compileShader(GL_VERTEX_SHADER, vertexShaderSource, vertShader, shaderError)) {
        std::cerr << shaderError << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    std::string fragSource = shaderFiles[0] == "fallback" ? fallbackFragmentShaderSource : loadShaderFile(shaderFiles[0], shaderError);
    if (!shaderError.empty()) {
        std::cerr << shaderError << std::endl;
        fragSource = fallbackFragmentShaderSource;
    }
    if (!compileShader(GL_FRAGMENT_SHADER, fragSource.c_str(), fragShader, shaderError)) {
        std::cerr << shaderError << std::endl;
        glDeleteShader(vertShader);
        fragSource = fallbackFragmentShaderSource;
        if (!compileShader(GL_FRAGMENT_SHADER, fragSource.c_str(), fragShader, shaderError)) {
            std::cerr << shaderError << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }
    }
    if (!linkProgram(vertShader, fragShader, shaderProgram, shaderError)) {
        std::cerr << shaderError << std::endl;
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    // Get uniform locations
    GLint iTimeLoc = glGetUniformLocation(shaderProgram, "iTime");
    GLint iResLoc = glGetUniformLocation(shaderProgram, "iResolution");

    // Setup full-screen quad
    float quadVertices[] = {
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

    // GUI variables
    int totalFrames = 1800;
    float desiredDuration = 30.0f;
    float slowdownFactor = 1.0f;
    int offWidth = OFF_WIDTH;
    int offHeight = OFF_HEIGHT;
    bool startOfflineRender = false;
    double lastFrameTime = glfwGetTime();
    float fps = 0.0f;
    std::string errorMessage = loadError;
    bool applyShader = false;
    std::string loadedShadersList;
    for (const auto& name : shaderNames) {
        loadedShadersList += name + "\n";
    }

    // Preview timing
    double previewStart = glfwGetTime();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Calculate FPS
        double currentTime = glfwGetTime();
        fps = 1.0f / static_cast<float>(currentTime - lastFrameTime);
        lastFrameTime = currentTime;

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // UI panel
        ImGui::SetNextWindowSize(ImVec2(400, 500));
        ImGui::Begin("Shader Controls");
        // std::cerr << "Rendering ImGui Shader Controls window\n";
        ImGui::Text("Shader Selection");
        ImGui::Combo("Shader", &currentShaderIndex, shaderNamesCStr.data(), shaderNamesCStr.size());
        ImGui::Text("Debug: Apply button follows");
        if (ImGui::Button("Apply Shader")) {
            applyShader = true;
            std::cerr << "Apply Shader button clicked\n";
        }

        // Display loaded shaders
        ImGui::Separator();
        ImGui::Text("Loaded Shaders:");
        ImGui::TextWrapped("%s", loadedShadersList.c_str());

        // Apply shader if button pressed
        if (applyShader) {
            applyShader = false;
            if (shaderProgram != 0) glDeleteProgram(shaderProgram);
            errorMessage.clear();
            if (!compileShader(GL_VERTEX_SHADER, vertexShaderSource, vertShader, shaderError)) {
                std::cerr << shaderError << std::endl;
                shaderProgram = 0;
                continue;
            }
            std::string newFragSource = shaderFiles[currentShaderIndex] == "fallback" ? fallbackFragmentShaderSource : loadShaderFile(shaderFiles[currentShaderIndex], errorMessage);
            if (!errorMessage.empty()) {
                std::cerr << errorMessage << std::endl;
                newFragSource = fallbackFragmentShaderSource;
            }
            std::cerr << "Shader content for " << shaderNames[currentShaderIndex] << ":\n" << newFragSource << "\n";
            if (!compileShader(GL_FRAGMENT_SHADER, newFragSource.c_str(), fragShader, shaderError)) {
                std::cerr << shaderError << std::endl;
                glDeleteShader(vertShader);
                newFragSource = fallbackFragmentShaderSource;
                if (!compileShader(GL_FRAGMENT_SHADER, newFragSource.c_str(), fragShader, shaderError)) {
                    std::cerr << shaderError << std::endl;
                    glDeleteShader(vertShader);
                    shaderProgram = 0;
                    continue;
                }
            }
            if (!linkProgram(vertShader, fragShader, shaderProgram, shaderError)) {
                std::cerr << shaderError << std::endl;
                glDeleteShader(vertShader);
                glDeleteShader(fragShader);
                shaderProgram = 0;
                continue;
            }
            glDeleteShader(vertShader);
            glDeleteShader(fragShader);
            iTimeLoc = glGetUniformLocation(shaderProgram, "iTime");
            iResLoc = glGetUniformLocation(shaderProgram, "iResolution");
            std::cerr << "Applied shader: " << shaderNames[currentShaderIndex] << ", iTimeLoc: " << iTimeLoc << ", iResLoc: " << iResLoc << "\n";
        }

        // Display errors
        if (!errorMessage.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error:");
            ImGui::TextWrapped("%s", errorMessage.c_str());
        }

        // Render settings
        ImGui::Separator();
        ImGui::Text("Render Settings");
        ImGui::Text("FPS: %.1f", fps);
        ImGui::InputInt("Render Width", &offWidth);
        ImGui::InputInt("Render Height", &offHeight);
        ImGui::InputInt("Total Frames", &totalFrames);
        ImGui::InputFloat("Duration (seconds)", &desiredDuration, 1.0f, 100.0f, "%.1f");
        ImGui::InputFloat("Slowdown Factor", &slowdownFactor, 0.1f, 10.0f, "%.2f");
        if (ImGui::Button("Start Offline Render")) {
            startOfflineRender = true;
        }
        ImGui::End();

        // Render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        if (shaderProgram != 0) {
            glUseProgram(shaderProgram);
            float elapsed = static_cast<float>(glfwGetTime() - previewStart);
            if (iTimeLoc != -1) glUniform1f(iTimeLoc, elapsed);
            if (iResLoc != -1) glUniform3f(iResLoc, static_cast<float>(WIN_WIDTH), static_cast<float>(WIN_HEIGHT), 1.0f);
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        // Check OpenGL errors
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            errorMessage = "OpenGL error: " + std::to_string(err);
            std::cerr << errorMessage << std::endl;
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || startOfflineRender) {
            break;
        }
    }

    // Offline rendering
    if (startOfflineRender && shaderProgram != 0) {
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
                                std::to_string(offWidth) + "x" + std::to_string(offHeight) +
                                " -framerate 60 -i - -c:v libx264 -pix_fmt yuv420p " + outputFile;

        // Offline Render Setup
        std::cout << "Starting offline render...\n";
        FILE* ffmpegPipe = popen(ffmpegCmd.c_str(), "w");
        if (!ffmpegPipe) {
            std::cerr << "Failed to open ffmpeg pipe.\n";
        } else {
            // Setup off-screen framebuffer
            GLuint fbo, texColorBuffer;
            glGenFramebuffers(1, &fbo);
            glGenTextures(1, &texColorBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glBindTexture(GL_TEXTURE_2D, texColorBuffer);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, offWidth, offHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Framebuffer incomplete!\n";
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Offline render loop
            std::vector<unsigned char> frameBuffer(offWidth * offHeight * 3);
            for (int frame = 0; frame < totalFrames; ++frame) {
                float simulatedTime = (frame / static_cast<float>(totalFrames - 1)) * desiredDuration * slowdownFactor;

                glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                glViewport(0, 0, offWidth, offHeight);
                glClear(GL_COLOR_BUFFER_BIT);
                glUseProgram(shaderProgram);
                if (iTimeLoc != -1) glUniform1f(iTimeLoc, simulatedTime);
                if (iResLoc != -1) glUniform3f(iResLoc, static_cast<float>(offWidth), static_cast<float>(offHeight), 1.0f);
                glBindVertexArray(VAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);
                glFinish();

                glReadPixels(0, 0, offWidth, offHeight, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());
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
        }
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    if (shaderProgram != 0) glDeleteProgram(shaderProgram);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
