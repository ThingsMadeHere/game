#pragma once
#include "raylib.h"
#include "../terrain/terrain.h"
#include "texture_atlas.h"
#include <cstdio>
#include <glad.h>

class GPUChunkRenderer {
private:
    Shader voxelShader;
    Model chunkModel;
    BlockTextureAtlas textureAtlas; // Contains all block type textures
    
    // Uniform locations
    int uDensityTexture;
    int uChunkPosition;
    int uVoxelSize;
    
    // Lighting uniform locations
    int uGrassTexture;
    int uLightDir;
    int uLightColor;
    int uAmbientColor;
    int uViewPos;
    
    // Default density texture (white = solid) - OpenGL GLuint for 3D texture
    GLuint densityTextureID;
    
    // Cached materials to avoid recreating every frame
    Material cachedTerrainMaterial;
    Material cachedShadowMaterial;
    
    // Grass texture for terrain
    Texture2D grassTexture;
    
public:
    void Init();
    void RenderChunk(const Chunk& chunk, const Camera3D& camera);
    void RenderChunkShadow(const Chunk& chunk, const Matrix& lightSpaceMatrix, Shader& shadowShader);
    void Cleanup();
};
