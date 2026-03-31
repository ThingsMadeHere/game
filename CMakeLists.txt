cmake_minimum_required(VERSION 3.10)
project(ModernTerrain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Raylib
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0
    )
    FetchContent_MakeAvailable(raylib)
endif()

# Get raylib include directories
if(raylib_FOUND)
    get_target_property(RAYLIB_INCLUDES raylib::raylib INTERFACE_INCLUDE_DIRECTORIES)
endif()

# Add Executable with all source files
add_executable(game 
    src/utils/main_new.cpp
    src/core/world.cpp
    src/terrain/noise.cpp
    src/rendering/marching_cubes.cpp
    src/rendering/gpu_renderer.cpp
    src/ui/game_menu.cpp
    src/ui/loading_screen.cpp
)

# Include raylib headers explicitly
if(RAYLIB_INCLUDES)
    target_include_directories(game PRIVATE ${RAYLIB_INCLUDES})
endif()

# Link Libraries (this will also include the necessary headers)
target_link_libraries(game PRIVATE 
    raylib 
    m  # Math library for Linux
)

# Optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(game PRIVATE -O3 -march=native)
endif()

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
