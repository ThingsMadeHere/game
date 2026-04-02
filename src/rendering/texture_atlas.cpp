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

// Texture generation functions with improved, more natural-looking textures

// Better pseudo-random noise function for textures
float TextureNoise(int x, int y, int seed) {
    float n = sinf((float)x * 12.9898f + (float)y * 78.233f + (float)seed) * 43758.5453f;
    return n - floorf(n);
}

// Smooth interpolation for natural gradients
float SmoothInterp(float t) {
    return t * t * (3.0f - 2.0f * t);
}

Color* GenerateGrassTexture(int size) {
    Color* pixels = new Color[size * size];
    
    // Base colors with better variation
    Color darkGreen = {34, 139, 34, 255};
    Color mediumGreen = {60, 160, 60, 255};
    Color lightGreen = {80, 180, 80, 255};
    Color earthBrown = {101, 67, 33, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Multi-layer noise for natural variation
            float n1 = TextureNoise(x, y, 1) * 0.5f + 0.5f;
            float n2 = TextureNoise(x * 2, y * 2, 2) * 0.3f + 0.3f;
            float n3 = TextureNoise(x * 4, y * 4, 3) * 0.2f + 0.2f;
            float noise = (n1 + n2 + n3) / 1.0f;
            
            // Add grass clumps pattern
            float clumpNoise = TextureNoise(x / 4, y / 4, 4);
            bool isClump = clumpNoise > 0.6f;
            
            Color pixel;
            if (noise < 0.15f && !isClump) {
                // Exposed dirt patches
                pixel = earthBrown;
            } else if (noise < 0.4f || isClump) {
                pixel = darkGreen;
            } else if (noise < 0.7f) {
                pixel = mediumGreen;
            } else {
                pixel = lightGreen;
            }
            
            // Add subtle highlights for depth
            if (TextureNoise(x + 1, y, 5) > 0.7f) {
                pixel.r = (unsigned char)fminf(255, pixel.r + 15);
                pixel.g = (unsigned char)fminf(255, pixel.g + 20);
                pixel.b = (unsigned char)fminf(255, pixel.b + 10);
            }
            
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateDirtTexture(int size) {
    Color* pixels = new Color[size * size];
    
    // Rich brown palette
    Color darkBrown = {78, 52, 28, 255};
    Color mediumBrown = {101, 67, 33, 255};
    Color lightBrown = {139, 90, 43, 255};
    Color reddishBrown = {120, 70, 40, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Layered noise for soil variation
            float n1 = TextureNoise(x, y, 10) * 0.5f + 0.5f;
            float n2 = TextureNoise(x * 3, y * 3, 11) * 0.3f + 0.3f;
            float noise = (n1 + n2) / 0.8f;
            
            Color pixel;
            if (noise < 0.25f) {
                pixel = darkBrown;
            } else if (noise < 0.5f) {
                pixel = mediumBrown;
            } else if (noise < 0.75f) {
                pixel = reddishBrown;
            } else {
                pixel = lightBrown;
            }
            
            // Add small pebbles and organic matter
            float speckle = TextureNoise(x * 5, y * 5, 12);
            if (speckle > 0.85f) {
                // Lighter mineral flecks
                pixel.r = (unsigned char)fminf(255, pixel.r + 40);
                pixel.g = (unsigned char)fminf(255, pixel.g + 30);
                pixel.b = (unsigned char)fminf(255, pixel.b + 20);
            } else if (speckle < 0.15f) {
                // Darker organic spots
                pixel.r = (unsigned char)fmaxf(0, pixel.r - 20);
                pixel.g = (unsigned char)fmaxf(0, pixel.g - 15);
                pixel.b = (unsigned char)fmaxf(0, pixel.b - 10);
            }
            
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateStoneTexture(int size) {
    Color* pixels = new Color[size * size];
    
    // Natural stone color palette
    Color darkGray = {80, 80, 85, 255};
    Color mediumGray = {110, 110, 115, 255};
    Color lightGray = {145, 145, 150, 255};
    Color bluishGray = {95, 100, 110, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Coarse noise for base variation
            float n1 = TextureNoise(x / 2, y / 2, 20) * 0.4f + 0.4f;
            float n2 = TextureNoise(x / 4, y / 4, 21) * 0.3f + 0.3f;
            float n3 = TextureNoise(x, y, 22) * 0.3f + 0.3f;
            float noise = (n1 + n2 + n3) / 1.0f;
            
            Color pixel;
            if (noise < 0.25f) {
                pixel = darkGray;
            } else if (noise < 0.5f) {
                pixel = bluishGray;
            } else if (noise < 0.75f) {
                pixel = mediumGray;
            } else {
                pixel = lightGray;
            }
            
            // Add mineral veins
            float veinNoise = TextureNoise(x / 8, y, 23);
            if (veinNoise > 0.85f) {
                // Quartz-like lighter streaks
                pixel.r = (unsigned char)fminf(255, pixel.r + 25);
                pixel.g = (unsigned char)fminf(255, pixel.g + 25);
                pixel.b = (unsigned char)fminf(255, pixel.b + 30);
            }
            
            // Subtle cracks and imperfections
            float crackNoise = TextureNoise(x * 3, y * 2, 24);
            if (crackNoise < 0.08f) {
                pixel.r = (unsigned char)fmaxf(0, pixel.r - 30);
                pixel.g = (unsigned char)fmaxf(0, pixel.g - 30);
                pixel.b = (unsigned char)fmaxf(0, pixel.b - 30);
            }
            
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateWoodTexture(int size) {
    Color* pixels = new Color[size * size];
    
    // Rich wood palette with natural variation
    Color darkWood = {78, 52, 28, 255};
    Color mediumWood = {101, 67, 33, 255};
    Color lightWood = {139, 90, 43, 255};
    Color warmWood = {120, 75, 45, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Create organic grain pattern using noise
            float baseGrain = TextureNoise(x / 2, y, 30) * 0.5f + 0.5f;
            float detailGrain = TextureNoise(x, y / 2, 31) * 0.3f + 0.3f;
            float grain = (baseGrain + detailGrain) / 0.8f;
            
            // Add growth rings effect
            float ringNoise = TextureNoise(x, y / 8, 32);
            bool isRing = ringNoise > 0.7f || ringNoise < 0.3f;
            
            Color pixel;
            if (grain < 0.25f) {
                pixel = darkWood;
            } else if (grain < 0.5f) {
                pixel = isRing ? warmWood : mediumWood;
            } else if (grain < 0.75f) {
                pixel = lightWood;
            } else {
                pixel = warmWood;
            }
            
            // Add subtle knots and imperfections
            float knotNoise = TextureNoise(x / 6, y / 6, 33);
            if (knotNoise > 0.92f) {
                // Dark knot
                pixel.r = (unsigned char)fmaxf(0, pixel.r - 30);
                pixel.g = (unsigned char)fmaxf(0, pixel.g - 25);
                pixel.b = (unsigned char)fmaxf(0, pixel.b - 20);
            }
            
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateSandTexture(int size) {
    Color* pixels = new Color[size * size];
    
    // Natural sand color palette
    Color lightSand = {245, 225, 195, 255};
    Color mediumSand = {238, 203, 173, 255};
    Color goldenSand = {244, 184, 96, 255};
    Color darkSand = {210, 160, 120, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Multi-layer noise for grainy texture
            float n1 = TextureNoise(x, y, 40) * 0.4f + 0.4f;
            float n2 = TextureNoise(x * 3, y * 3, 41) * 0.3f + 0.3f;
            float n3 = TextureNoise(x * 6, y * 6, 42) * 0.3f + 0.3f;
            float noise = (n1 + n2 + n3) / 1.0f;
            
            Color pixel;
            if (noise < 0.2f) {
                pixel = darkSand;
            } else if (noise < 0.5f) {
                pixel = mediumSand;
            } else if (noise < 0.75f) {
                pixel = goldenSand;
            } else {
                pixel = lightSand;
            }
            
            // Add tiny mineral flecks
            float fleckNoise = TextureNoise(x * 8, y * 8, 43);
            if (fleckNoise > 0.9f) {
                // Light quartz-like flecks
                pixel.r = (unsigned char)fminf(255, pixel.r + 20);
                pixel.g = (unsigned char)fminf(255, pixel.g + 15);
                pixel.b = (unsigned char)fminf(255, pixel.b + 10);
            } else if (fleckNoise < 0.1f) {
                // Dark mineral grains
                pixel.r = (unsigned char)fmaxf(0, pixel.r - 30);
                pixel.g = (unsigned char)fmaxf(0, pixel.g - 25);
                pixel.b = (unsigned char)fmaxf(0, pixel.b - 20);
            }
            
            pixels[y * size + x] = pixel;
        }
    }
    return pixels;
}

Color* GenerateBrickTexture(int size) {
    Color* pixels = new Color[size * size];
    
    // Rich brick palette
    Color brickRed = {158, 34, 34, 255};
    Color brickDark = {119, 26, 26, 255};
    Color brickLight = {178, 54, 54, 255};
    Color mortar = {160, 160, 155, 255};
    
    int brickHeight = size / 4;
    int brickWidth = size / 2;
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int row = y / brickHeight;
            int offset = (row % 2) * (brickWidth / 2);
            int brickX = (x + offset) % brickWidth;
            
            // Mortar lines (between bricks)
            bool isMortarVertical = (brickX == 0 || brickX == brickWidth - 1);
            bool isMortarHorizontal = (y % brickHeight == 0);
            
            if (isMortarVertical || isMortarHorizontal) {
                // Mortar with slight variation
                float mortarVar = TextureNoise(x, y, 50) * 0.15f + 0.85f;
                pixels[y * size + x] = {
                    (unsigned char)(mortar.r * mortarVar),
                    (unsigned char)(mortar.g * mortarVar),
                    (unsigned char)(mortar.b * mortarVar),
                    255
                };
            } else {
                // Brick body with weathering
                float brickNoise = TextureNoise(x / 3, y / 3, 51);
                
                Color pixel;
                if (brickNoise < 0.3f) {
                    pixel = brickDark;
                } else if (brickNoise < 0.7f) {
                    pixel = brickRed;
                } else {
                    pixel = brickLight;
                }
                
                // Add surface wear and texture
                float wearNoise = TextureNoise(x, y, 52);
                if (wearNoise > 0.85f) {
                    // Lighter worn spots
                    pixel.r = (unsigned char)fminf(255, pixel.r + 25);
                    pixel.g = (unsigned char)fminf(255, pixel.g + 20);
                    pixel.b = (unsigned char)fminf(255, pixel.b + 15);
                } else if (wearNoise < 0.15f) {
                    // Darker patches
                    pixel.r = (unsigned char)fmaxf(0, pixel.r - 20);
                    pixel.g = (unsigned char)fmaxf(0, pixel.g - 15);
                    pixel.b = (unsigned char)fmaxf(0, pixel.b - 10);
                }
                
                pixels[y * size + x] = pixel;
            }
        }
    }
    return pixels;
}

Color* GenerateMetalTexture(int size) {
    Color* pixels = new Color[size * size];
    
    // Brushed metal palette
    Color darkMetal = {140, 140, 145, 255};
    Color mediumMetal = {170, 170, 175, 255};
    Color lightMetal = {200, 200, 205, 255};
    Color highlightMetal = {220, 220, 225, 255};
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Base metal variation
            float baseNoise = TextureNoise(x / 4, y / 4, 60) * 0.4f + 0.4f;
            
            // Horizontal brushed streaks
            float streakNoise = TextureNoise(x * 2, y / 2, 61) * 0.3f + 0.3f;
            
            float combined = (baseNoise + streakNoise) / 0.7f;
            
            Color pixel;
            if (combined < 0.25f) {
                pixel = darkMetal;
            } else if (combined < 0.5f) {
                pixel = mediumMetal;
            } else if (combined < 0.75f) {
                pixel = lightMetal;
            } else {
                pixel = highlightMetal;
            }
            
            // Add subtle horizontal brush marks
            float brushNoise = TextureNoise(x, y, 62);
            if (brushNoise > 0.6f && brushNoise < 0.65f) {
                pixel.r = (unsigned char)fminf(255, pixel.r + 15);
                pixel.g = (unsigned char)fminf(255, pixel.g + 15);
                pixel.b = (unsigned char)fminf(255, pixel.b + 18);
            }
            
            // Occasional scratches
            float scratchNoise = TextureNoise(x * 4, y, 63);
            if (scratchNoise > 0.95f) {
                pixel.r = (unsigned char)fminf(255, pixel.r + 30);
                pixel.g = (unsigned char)fminf(255, pixel.g + 30);
                pixel.b = (unsigned char)fminf(255, pixel.b + 35);
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
            // Semi-transparent blue-tinted glass
            unsigned char alpha = 150;
            
            // Make edges slightly more opaque for subtle frame effect
            int distFromEdge = fminf(fminf(x, y), fminf(size - 1 - x, size - 1 - y));
            if (distFromEdge < 3) {
                alpha = 180;
            }
            
            // Subtle gradient for depth
            float gradient = (float)(x + y) / (size * 2);
            
            // Glass tint variation
            float tintNoise = TextureNoise(x * 2, y * 2, 70) * 0.15f + 0.85f;
            
            unsigned char baseValue = (unsigned char)(180 + gradient * 40);
            unsigned char r = (unsigned char)(baseValue * tintNoise * 0.95f);
            unsigned char g = (unsigned char)(baseValue * tintNoise);
            unsigned char b = (unsigned char)fminf(255, baseValue * tintNoise * 1.05f + 20);
            
            pixels[y * size + x] = {r, g, b, alpha};
        }
    }
    return pixels;
}
