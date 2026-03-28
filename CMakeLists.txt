cmake_minimum_required(VERSION 3.10)
project(PlanetDemo)

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

# Find JSON and Threads
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

# Add Executable
add_executable(game
    src/main.cpp
    src/Planet.cpp
    src/MarchingCubes.cpp
)

# Link Libraries
target_link_libraries(game PRIVATE 
    raylib 
    nlohmann_json::nlohmann_json 
    Threads::Threads
    m  # Math library for Linux
)

# Optimizations for 2017 hardware
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(game PRIVATE -O3 -march=native)
endif()

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
