cmake_minimum_required(VERSION 3.15)

# Set all libs to be static
option(BUILD_SHARED_LIBS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#if building for IDE, enable folders
if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

project(Campfire
    #    VERSION 0.1
    #    DESCRIPTION "Game Engine"
    #    LANGUAGES CXX
)

file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if (EXISTS "${LOC_PATH}")
  message(FATAL_ERROR "build must be out of source")
endif()

add_subdirectory(Engine)
add_subdirectory(Vendor)
add_subdirectory(Editor)
#add_subdirectory(Game)
