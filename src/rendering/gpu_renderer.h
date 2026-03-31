#pragma once
#include "../terrain/terrain.h"
#include "raylib.h"

class GPUChunkRenderer {
private:
    Shader voxelShader;
    Model chunkModel;  // Single cube model for all chunks
    
public:
    void Init();
    void RenderChunk(const Chunk& chunk, const Camera3D& camera);
    void Cleanup();
};
