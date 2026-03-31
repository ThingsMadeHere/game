#pragma once
#include "raylib.h"
#include "../terrain/terrain.h"

// Texture atlas for all block types
// Layout: 4x4 grid of 32x32 textures = 128x128 atlas
class BlockTextureAtlas {
private:
    Texture2D atlasTexture;
    static const int BLOCK_SIZE = 32;
    static const int ATLAS_SIZE = 128; // 4x4 grid
    
public:
    void Init();
    void Cleanup();
    
    Texture2D GetTexture() const { return atlasTexture; }
    
    // Get UV coordinates for a specific block type
    // Returns bottom-left and top-right UV coordinates
    void GetBlockUVs(BlockType type, float& u1, float& v1, float& u2, float& v2) const;
    
    // Get UV offset for a block type
    Vector2 GetBlockUVOffset(BlockType type) const;
    
    static int GetBlockIndex(BlockType type);
};

// Generate textures for each block type
Color* GenerateGrassTexture(int size);
Color* GenerateDirtTexture(int size);
Color* GenerateStoneTexture(int size);
Color* GenerateWoodTexture(int size);
Color* GenerateSandTexture(int size);
Color* GenerateBrickTexture(int size);
Color* GenerateMetalTexture(int size);
Color* GenerateGlassTexture(int size);
