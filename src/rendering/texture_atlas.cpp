#include "texture_atlas.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>

void BlockTextureAtlas::Init() {
    const int blockSize = BLOCK_SIZE;
    const int atlasSize = ATLAS_SIZE;
    Color* atlasPixels = new Color[atlasSize * atlasSize];
    
    // Generate textures for each block type
    Color* grass = GenerateGrassTexture(blockSize);
    Color* dirt = GenerateDirtTexture(blockSize);
    Color* stone = GenerateStoneTexture(blockSize);
    Color* wood = GenerateWoodTexture(blockSize);
    Color* sand = GenerateSandTexture(blockSize);
    Color* brick = GenerateBrickTexture(blockSize);
    Color* metal = GenerateMetalTexture(blockSize);
    Color* glass = GenerateGlassTexture(blockSize);
    
    // Copy each texture into the atlas (4x4 grid)
    // Row 0: Grass, Dirt, Stone, Wood
    // Row 1: Sand, Brick, Metal, Glass
    Color* textures[8] = {grass, dirt, stone, wood, sand, brick, metal, glass};
    
    for (int i = 0; i < 8; i++) {
        int row = i / 4;
        int col = i % 4;
        
        for (int y = 0; y < blockSize; y++) {
            for (int x = 0; x < blockSize; x++) {
                int atlasX = col * blockSize + x;
                int atlasY = row * blockSize + y;
                atlasPixels[atlasY * atlasSize + atlasX] = textures[i][y * blockSize + x];
            }
        }
    }
    
    // Create image and load texture
    Image atlasImage = {
        atlasPixels,
        atlasSize,
        atlasSize,
        1,
        PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    
    atlasTexture = LoadTextureFromImage(atlasImage);
    SetTextureWrap(atlasTexture, TEXTURE_WRAP_CLAMP);
    
    UnloadImage(atlasImage);
    
    // Clean up temporary buffers
    delete[] grass;
    delete[] dirt;
    delete[] stone;
    delete[] wood;
    delete[] sand;
    delete[] brick;
    delete[] metal;
    delete[] glass;
    
    fprintf(stderr, "Created block texture atlas: %dx%d\n", atlasSize, atlasSize);
}

void BlockTextureAtlas::Cleanup() {
    if (atlasTexture.id != 0) {
        UnloadTexture(atlasTexture);
        atlasTexture = {0};
    }
}

int BlockTextureAtlas::GetBlockIndex(BlockType type) {
    switch (type) {
        case BlockType::GRASS: return 0;
        case BlockType::DIRT: return 1;
        case BlockType::STONE: return 2;
        case BlockType::WOOD: return 3;
        case BlockType::SAND: return 4;
        case BlockType::BRICK: return 5;
        case BlockType::METAL: return 6;
        case BlockType::GLASS: return 7;
        default: return 0;
    }
}

void BlockTextureAtlas::GetBlockUVs(BlockType type, float& u1, float& v1, float& u2, float& v2) const {
    int index = GetBlockIndex(type);
    int row = index / 4;
    int col = index % 4;
    
    float cellSize = 1.0f / 4.0f; // Each cell is 1/4 of the atlas
    float padding = 0.01f; // Small padding to avoid bleeding
    
    u1 = col * cellSize + padding;
    v1 = row * cellSize + padding;
    u2 = (col + 1) * cellSize - padding;
    v2 = (row + 1) * cellSize - padding;
}

Vector2 BlockTextureAtlas::GetBlockUVOffset(BlockType type) const {
    int index = GetBlockIndex(type);
    int row = index / 4;
    int col = index % 4;
    
    return {(float)col / 4.0f, (float)row / 4.0f};
}

// Texture generation functions
Color* GenerateGrassTexture(int size) {
    Color* pixels = new Color[size * size];
    Color darkGreen = {34, 139, 34, 255};
    Color lightGreen = {50, 205, 50, 255};
    Color brown = {139, 69, 19, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float noise = (float)(rand() % 100) / 100.0f;
            
            Color pixel;
            if (noise < 0.1f) pixel = brown;
            else if (noise < 0.6f) pixel = darkGreen;
            else pixel = lightGreen;
            
            if ((x + y) % 3 == 0 && noise > 0.3f) {
                pixel.r = (unsigned char)fminf(255, pixel.r + 20);
                pixel.g = (unsigned char)fminf(255, pixel.g + 30);
            }
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateDirtTexture(int size) {
    Color* pixels = new Color[size * size];
    Color darkBrown = {101, 67, 33, 255};
    Color mediumBrown = {139, 69, 19, 255};
    Color lightBrown = {160, 82, 45, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float noise = (float)(rand() % 100) / 100.0f;
            
            Color pixel;
            if (noise < 0.3f) pixel = darkBrown;
            else if (noise < 0.7f) pixel = mediumBrown;
            else pixel = lightBrown;
            
            // Add speckles
            if (rand() % 10 == 0) {
                pixel.r = (unsigned char)fminf(255, pixel.r + 30);
            }
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateStoneTexture(int size) {
    Color* pixels = new Color[size * size];
    Color darkGray = {105, 105, 105, 255};
    Color mediumGray = {128, 128, 128, 255};
    Color lightGray = {169, 169, 169, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float noise = (float)(rand() % 100) / 100.0f;
            
            Color pixel;
            if (noise < 0.3f) pixel = darkGray;
            else if (noise < 0.7f) pixel = mediumGray;
            else pixel = lightGray;
            
            // Add cracks
            if (rand() % 20 == 0) {
                pixel = {64, 64, 64, 255};
            }
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateWoodTexture(int size) {
    Color* pixels = new Color[size * size];
    Color darkWood = {101, 67, 33, 255};
    Color mediumWood = {139, 90, 43, 255};
    Color lightWood = {160, 120, 60, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Create wood grain pattern
            float grain = sinf(y * 0.5f) * 0.3f + (float)(rand() % 30) / 100.0f;
            
            Color pixel;
            if (grain < 0.2f) pixel = darkWood;
            else if (grain < 0.6f) pixel = mediumWood;
            else pixel = lightWood;
            
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateSandTexture(int size) {
    Color* pixels = new Color[size * size];
    Color sand1 = {238, 203, 173, 255};
    Color sand2 = {244, 164, 96, 255};
    Color sand3 = {210, 180, 140, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float noise = (float)(rand() % 100) / 100.0f;
            
            Color pixel;
            if (noise < 0.3f) pixel = sand1;
            else if (noise < 0.7f) pixel = sand2;
            else pixel = sand3;
            
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateBrickTexture(int size) {
    Color* pixels = new Color[size * size];
    Color brickRed = {178, 34, 34, 255};
    Color brickDark = {139, 26, 26, 255};
    Color mortar = {192, 192, 192, 255};
    
    int brickHeight = size / 4;
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int row = y / brickHeight;
            int offset = (row % 2) * (size / 8);
            int brickX = (x + offset) % (size / 2);
            
            // Mortar lines
            if (y % brickHeight == 0 || brickX == 0) {
                pixels[y * size + x] = mortar;
            } else {
                float noise = (float)(rand() % 30) / 100.0f;
                Color pixel = brickRed;
                pixel.r = (unsigned char)fmaxf(0, fminf(255, pixel.r - noise * 50));
                pixel.g = (unsigned char)fmaxf(0, fminf(255, pixel.g - noise * 50));
                pixel.b = (unsigned char)fmaxf(0, fminf(255, pixel.b - noise * 50));
                pixels[y * size + x] = pixel;
            }
        }
    }
    return pixels;
}

Color* GenerateMetalTexture(int size) {
    Color* pixels = new Color[size * size];
    Color metal1 = {192, 192, 192, 255};
    Color metal2 = {169, 169, 169, 255};
    Color metal3 = {211, 211, 211, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float noise = (float)(rand() % 100) / 100.0f;
            
            Color pixel;
            if (noise < 0.3f) pixel = metal1;
            else if (noise < 0.7f) pixel = metal2;
            else pixel = metal3;
            
            // Add brushed metal streaks
            if (y % 4 == 0) {
                pixel.r = (unsigned char)fminf(255, pixel.r + 20);
                pixel.g = (unsigned char)fminf(255, pixel.g + 20);
                pixel.b = (unsigned char)fminf(255, pixel.b + 20);
            }
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateGlassTexture(int size) {
    Color* pixels = new Color[size * size];
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Semi-transparent blue-ish
            unsigned char alpha = 180;
            // Make edges more opaque for frame effect
            if (x < 2 || x >= size - 2 || y < 2 || y >= size - 2) {
                alpha = 220;
            }
            // Add some variation
            float noise = (float)(rand() % 30) / 100.0f;
            unsigned char value = (unsigned char)(200 + noise * 55);
            
            pixels[y * size + x] = {value, value, 255, alpha};
        }
    }
    return pixels;
}
