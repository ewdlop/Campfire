# Campfire
![Ubuntu build](https://github.com/danhuynh0803/Campfire/workflows/Ubuntu%20build/badge.svg)
![Windows build](https://github.com/danhuynh0803/Campfire/workflows/Windows%20build/badge.svg)
![CodeQL Advanced](https://github.com/danhuynh0803/Campfire/actions/workflows/codeql.yml/badge.svg)

## Summary
Campfire is an in-progress game engine built using OpenGL and Vulkan. Most of the engine's features are currently disabled, as we work through switching to Vulkan, but are available on the [OpenGL branch](https://github.com/danhuynh0803/Campfire/tree/OpenGL).

## Sample Images
![SampleScene](https://i.imgur.com/jZDVC6l.jpg)

## Realtime RT
![RT-Scene](https://i.imgur.com/kmzx7xv.png)

## Dependencies
Vulkan SDK: [1.2.148.1](https://vulkan.lunarg.com/sdk/home)

## Build Instructions
```bash
git clone --recursive https://github.com/danhuynh0803/Campfire
cd Campfire
cd Build
```

Now generate a project file or makefile for your platform. If you want to use a particular IDE, make sure it is installed; don't forget to set the Start-Up Project in Visual Studio or the Target in Xcode.

```bash
# UNIX Makefile
cmake ..
make -j4

# Microsoft Windows
cmake -G "Visual Studio 16 2019" ..
```
## Library
Functionality           | Library
----------------------- | ------------------------------------------
Mesh Loading            | [assimp](https://github.com/assimp/assimp)
Physics                 | [bullet](https://github.com/bulletphysics/bullet3)
OpenGL Function Loader  | [glad](https://github.com/Dav1dde/glad)
Windowing and Input     | [glfw](https://github.com/glfw/glfw)
OpenGL Mathematics      | [glm](https://github.com/g-truc/glm)
Texture Loading         | [stb](https://github.com/nothings/stb)
Logging                 | [spdlog](https://github.com/gabime/spdlog)
Scene Loading/Saving    | [json](https://github.com/nlohmann/json)
GUI                     | [imgui](https://github.com/ocornut/imgui)
Audio                   | [FMOD](https://www.fmod.com/studio)
Scripting               | [Lua 5.4.0](http://www.lua.org/download.html)
Entity Component System | [entt](https://github.com/skypjack/entt)




## Contributions by ewdlop

This section details the specific contributions made to the Campfire game engine project.

### Editor Layer (`Editor/`)
**What:** Implemented the core editor infrastructure for the game engine  
**Where:** `Editor/src/EditorLayer.cpp`, `Editor/include/EditorLayer.h`  
**Why:** To provide a user-friendly interface for game development, enabling scene management, user input processing, and real-time rendering visualization

### Vulkan Rendering Layer (`Editor/`, `Engine/Renderer/Vulkan/`)
**What:** Developed the Vulkan-based rendering layer for modern graphics API support  
**Where:** `Editor/src/VulkanLayer.cpp`, `Editor/include/VulkanLayer.h`, `Engine/Renderer/Vulkan/`  
**Why:** To transition from OpenGL to Vulkan for improved performance, modern graphics features, and better hardware utilization

### Editor UI Widgets (`Editor/Widgets/`)
**What:** Created interactive UI components for editor functionality  
**Where:**
- Hierarchy Widget: `Editor/src/Widgets/HierarchyWidget.cpp` - Scene hierarchy visualization and management
- Inspector Widget: `Editor/src/Widgets/InspectorWidget.cpp` - Entity property inspection and modification
- Transform Widget: `Editor/src/Widgets/TransformWidget.cpp` - Entity transformation controls
- Asset Browser: `Editor/src/Widgets/AssetBrowser.cpp` - Asset management and browsing interface  
**Why:** To provide intuitive tools for scene editing, entity manipulation, and asset management

### Particle Effect System (`Engine/Particles/`)
**What:** Implemented a particle system for visual effects  
**Where:** `Engine/Particles/src/ParticleSystem.cpp`, `Engine/Particles/include/Particles/`  
**Why:** To enable dynamic visual effects like fire, smoke, explosions, and other particle-based phenomena in the engine

### Lua Scripting Integration (`Engine/Scripting/`)
**What:** Integrated Lua 5.4.0 scripting capabilities into the engine  
**Where:** `Engine/Scripting/include/Scripting/`, `Engine/Events/include/Events/LuaEvent.h`  
**Why:** To allow game logic to be written in Lua scripts, providing flexibility and rapid iteration without recompiling the engine

### File System Operations
**What:** Added file system utility functions and operations  
**Why:** To support asset loading, scene serialization, and editor file management capabilities

