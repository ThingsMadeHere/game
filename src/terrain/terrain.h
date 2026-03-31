#pragma once
#include "raylib.h"
#include <vector>
#include <unordered_map>

// Constants
const int CHUNK_SIZE = 32;        // Balanced size for performance
const int CHUNK_HEIGHT = 32;      // Balanced height for performance
const int RENDER_DISTANCE = 4;   // Reduced to 4 for immediate performance boost
const float VOXEL_SIZE = 0.5f;    // Increase voxel size to reduce count

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
