#pragma once
#include "raylib.h"
#include <vector>
#include <unordered_map>

// Constants
const int CHUNK_SIZE = 32;        // Balanced size for performance
const int CHUNK_HEIGHT = 32;      // Balanced height for performance
const int RENDER_DISTANCE = 4;   // Reduced to 4 for immediate performance boost
const float VOXEL_SIZE = 0.5f;    // Increase voxel size to reduce count

// Material/Block types
enum class BlockType : unsigned char {
    AIR = 0,
    GRASS = 1,
    DIRT = 2,
    STONE = 3,
    WOOD = 4,
    SAND = 5,
    WATER = 6,
    LEAVES = 7,
    BRICK = 8,
    METAL = 9,
    GLASS = 10,
    COUNT
};

// Block properties
struct BlockProperties {
    Color color;
    bool transparent;
    float hardness; // Mining hardness
};

// Get properties for a block type
inline BlockProperties GetBlockProperties(BlockType type) {
    switch (type) {
        case BlockType::GRASS:  return {{34, 139, 34, 255}, false, 1.0f};
        case BlockType::DIRT:   return {{139, 69, 19, 255}, false, 0.8f};
        case BlockType::STONE:  return {{128, 128, 128, 255}, false, 3.0f};
        case BlockType::WOOD:   return {{139, 90, 43, 255}, false, 2.0f};
        case BlockType::SAND:   return {{238, 203, 173, 255}, false, 0.5f};
        case BlockType::WATER:  return {{65, 105, 225, 180}, true, 0.0f};
        case BlockType::LEAVES: return {{34, 100, 34, 255}, true, 0.3f};
        case BlockType::BRICK:  return {{178, 34, 34, 255}, false, 2.5f};
        case BlockType::METAL:  return {{192, 192, 192, 255}, false, 5.0f};
        case BlockType::GLASS:  return {{200, 200, 255, 100}, true, 0.5f};
        default:                return {{255, 255, 255, 255}, false, 1.0f};
    }
}

// Voxel data
struct Voxel {
    float density;
    unsigned char material; // Material type (0-255)
};

// Chunk data
struct Chunk {
    int cx, cy, cz;
    Voxel voxels[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE]; // Now 32x32x32
    Mesh mesh = {0};
    Texture2D densityTexture = {0}; // 3D texture storing voxel density (black=air, white=block)
    bool needsUpdate = false;
    bool meshGenerated = false; // Track if mesh has been generated
    bool densityTextureGenerated = false; // Track if density texture has been generated
    
    Chunk(int x, int y, int z) : cx(x), cy(y), cz(z), meshGenerated(false), densityTextureGenerated(false) {
        for (int i = 0; i < CHUNK_SIZE; i++) {
            for (int j = 0; j < CHUNK_HEIGHT; j++) {
                for (int k = 0; k < CHUNK_SIZE; k++) {
                    voxels[i][j][k].density = 0.0f;
                    voxels[i][j][k].material = 0;
                }
            }
        }
    }
    
    float GetDensity(int x, int y, int z) const {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
            return voxels[x][y][z].density;
        }
        return 0.0f;
    }
    
    float GetNeighborDensity(int x, int y, int z) const {
        // For chunk boundaries, return a reasonable default (air)
        // This prevents gaps by ensuring consistent values at boundaries
        return 0.0f;
    }
    
    void SetDensity(int x, int y, int z, float density) {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
            voxels[x][y][z].density = density;
            needsUpdate = true; // Mark as dirty when voxel changes
        }
    }
    
    unsigned char GetMaterial(int x, int y, int z) const {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
            return voxels[x][y][z].material;
        }
        return 0;
    }
    
    void SetMaterial(int x, int y, int z, unsigned char material) {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
            voxels[x][y][z].material = material;
            needsUpdate = true; // Mark as dirty when material changes
        }
    }
    
    void GenerateDensityTexture();
};

// Hash function for chunk keys
struct ChunkKey {
    int cx, cy, cz;
    
    bool operator==(const ChunkKey& other) const {
        return cx == other.cx && cy == other.cy && cz == other.cz;
    }
};

// Better hash function to reduce collisions
namespace std {
    template<> struct hash<ChunkKey> {
        size_t operator()(const ChunkKey& key) const {
            // Use boost::hash_combine style for better distribution
            size_t seed = 0;
            seed ^= hash<int>{}(key.cx) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hash<int>{}(key.cy) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hash<int>{}(key.cz) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
}
