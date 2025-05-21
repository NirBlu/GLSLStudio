
# GLSLStudio

GLSLStudio is a C++ application for viewing and rendering GLSL fragment shaders, built with OpenGL and Dear ImGui. It loads shaders from `.txt` files, allowing real-time preview and high-quality video rendering in 4K at 60 FPS. This tool was used to create the shader videos showcased in this [YouTube playlist](https://www.youtube.com/playlist?list=PLKFEqRrvMUF2EEbt6x3UWMniV6wtwYGSg).

## Features
- **Real-time Shader Preview**: Load and preview GLSL fragment shaders in a 4K window.
- **4K Video Rendering**: Export shader animations as `.mp4` files at 3840x2160 resolution and 60 FPS.
- **Shadertoy Workflow**: Convert Shadertoy shaders to the required format using AI tools like Grok or ChatGPT.
- **ImGui Interface**: User-friendly controls for selecting shaders, adjusting render settings, and starting offline renders.
- **Cross-Promotion**: Check out my related project, [Midimaker](https://youtube.com/link-to-midimaker-video) (demo link to be updated).

## Workflow: Using Shadertoy Shaders
1. **Find a Shader**: Visit [Shadertoy](https://www.shadertoy.com/) and select a shader you like (e.g., copy the fragment shader code).
2. **Translate the Shader**:
   - Paste the Shadertoy code into an AI tool like [Grok](https://grok.com/) or ChatGPT.
   - Ask it to convert the shader to match the format of the example shader in `shaders/example.txt`. For instance, prompt:
     _"Convert this Shadertoy GLSL code to the format used in this example: [paste example.txt content]. Ensure it uses `iTime` for time and `iResolution` for resolution."_
   - Save the AI-translated shader as a `.txt` file (e.g., `myshader.txt`) in the `shaders/` directory.
3. **Run GLSLStudio**:
   - Compile and run the application (see [Compilation](#compilation)).
   - Select your shader from the ImGui dropdown and click "Apply Shader" to preview.
4. **Render Video**:
   - Adjust settings (e.g., duration, resolution) in the ImGui panel.
   - Click "Start Offline Render" to generate a 4K 60 FPS `.mp4` file.

## Prerequisites
To compile and run GLSLStudio, you need:
- **g++**: C++17-compatible compiler (e.g., GCC).
- **GLFW**: Library for OpenGL context and window management.
- **GLAD**: OpenGL function loader (included in the repo).
- **ImGui**: Dear ImGui library for the UI (included in the repo).
- **OpenGL**: Version 3.3 or higher.
- **libstdc++fs**: For filesystem operations (usually included with g++).
- **FFmpeg**: For offline video rendering (ensure it’s installed and accessible in your PATH).

On Ubuntu/Debian, install dependencies with:
```bash
sudo apt update
sudo apt install g++ libglfw3-dev libgl1-mesa-dev ffmpeg
```

On macOS (using Homebrew):
```bash
brew install g++ glfw glew ffmpeg
```

## Compilation
Clone the repository and compile the application using the following command:
```bash
g++ -std=c++17 main.cpp glad/glad.c imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_widgets.cpp imgui/imgui_tables.cpp imgui/imgui_impl_glfw.cpp imgui/imgui_impl_opengl3.cpp -o shader_preview -Iimgui -Iglad -DIMGUI_IMPL_OPENGL_LOADER_GLAD -lglfw -ldl -lGL -lstdc++fs > log.txt 2>&1
```

This generates an executable named `shader_preview` and redirects compilation output to `log.txt`.

### Troubleshooting Compilation
- If no `shader_preview` file is generated, check `log.txt` for errors:
  - **Missing libraries**: Ensure GLFW, OpenGL, and FFmpeg are installed.
  - **Include path issues**: Verify that `imgui/` and `glad/` directories are in the project root.
  - **C++17 support**: Confirm your g++ version supports `-std=c++17` (run `g++ --version`).
- Example: If you see `cannot find -lglfw`, install `libglfw3-dev` (see [Prerequisites](#prerequisites)).

## Usage
1. Place your GLSL fragment shaders as `.txt` files in the `shaders/` directory (see [Workflow](#workflow-using-shadertoy-shaders)).
2. Run the application:
   ```bash
   ./shader_preview
   ```
3. Use the ImGui interface to:
   - Select a shader from the dropdown.
   - Click "Apply Shader" to preview.
   - Adjust render settings (e.g., resolution, duration, slowdown factor).
   - Click "Start Offline Render" to export a 4K video.
4. Press `ESC` to exit or start the offline render.

## Folder Structure
```
GLSLStudio/
├── main.cpp              # Main application code
├── glad.c
├── imgui.h
├── imgui.ini
├── main_noui.cpp
├── stb_image_write.h
├── glad/                 # GLAD OpenGL loader
├── imgui/                # Dear ImGui library
├── shaders/              # Directory for .txt shader files
├── README.md             # This file
├── log.txt               # Compilation log (generated after compiling)
├── shader_preview        # Executable (generated after compiling)
```

## Demos
- **GLSLStudio Playlist**: [YouTube Playlist](https://www.youtube.com/playlist?list=PLKFEqRrvMUF2EEbt6x3UWMniV6wtwYGSg)
- **Midimaker Demo**: that is being used to generate the procedural music in the background of those YouTube videos. rendered using [Vital](https://vital.audio/) in [Ardour](https://ardour.org/)

## Contributing
Feel free to submit issues or pull requests for new features, bug fixes, or shader examples. Ensure changes are tested and compatible with the existing setup.

## License
[MIT License](LICENSE)
