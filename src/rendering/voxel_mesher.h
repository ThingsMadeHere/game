#pragma once
#include "../terrain/terrain.h"
#include <vector>
#include <array>
#include <cstdint>

// High-performance voxel mesher with greedy meshing and ambient occlusion
// Reduces vertex count by 80-95% compared to naive meshing
class VoxelMesher {
public:
    // Generate optimized mesh from chunk data using greedy meshing
    static Mesh GenerateChunkMesh(const Chunk& chunk, 
                                   const std::unordered_map<long long, Chunk>* neighborChunks = nullptr);
    
private:
    // Vertex format matching Raylib's expectations
    struct Vertex {
        float x, y, z;          // Position
        float nx, ny, nz;       // Normal
        float u, v;             // Texture coordinates
        uint8_t r, g, b, a;     // Color + AO (packed)
    };
    
    // Greedy meshing data for a single slice
    struct MaskSlice {
        bool exists[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE];
        uint8_t blockIds[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE];
    };
    
    // Greedy mesh generation for one axis direction
    static void GenerateGreedyMesh(
        const Chunk& chunk,
        std::vector<Vertex>& vertices,
        int axis,              // 0=Y (top/bottom), 1=X (right/left), 2=Z (front/back)
        bool positiveDir,      // true = positive direction, false = negative
        const std::unordered_map<long long, Chunk>* neighborChunks
    );
    
    // Build mask for greedy meshing (which voxels have exposed faces)
    static void BuildFaceMask(
        const Chunk& chunk,
        bool* mask,
        int axis,
        bool positiveDir,
        const std::unordered_map<long long, Chunk>* neighborChunks
    );
    
    // Add a merged quad to the vertex buffer
    static void AddMergedQuad(
        std::vector<Vertex>& vertices,
        float x, float y, float z,
        float width, float height,
        int axis,
        bool positiveDir,
        BlockType blockType,
        const std::array<uint8_t, 4>& aoValues
    );
    
    // Calculate ambient occlusion at a corner
    static uint8_t ComputeAO(
        const Chunk& chunk,
        int x, int y, int z,
        int axis,
        bool positiveDir,
        int cornerIndex,
        const std::unordered_map<long long, Chunk>* neighborChunks
    );
    
    // Get texture UV for a block face
    static void GetTextureUV(BlockType blockType, int face, float& u, float& v, float& cellSize);
};
