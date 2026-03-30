#pragma once
#include "raylib.h"
#include "terrain.h"

class GPUChunkRenderer {
private:
    Shader voxelShader;
    Model chunkModel;  // Single cube model for all chunks
    
public:
    void Init();
    void RenderChunk(const Chunk& chunk, const Camera3D& camera);
    void Cleanup();
};
