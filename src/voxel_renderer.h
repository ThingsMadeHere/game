#pragma once
#include "raylib.h"
#include "terrain.h"

class VoxelRenderer {
private:
    // Block colors for different materials
    static const Color blockColors[12];
    
public:
    static void Init();
    static void RenderChunkFromDensity(const Chunk& chunk);
    static Color GetBlockColor(unsigned char material);
};
