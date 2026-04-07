#pragma once
#include "../terrain/terrain.h"
#include <vector>
#include <array>

// Optimized voxel mesh generator using greedy meshing
// Merges adjacent faces to reduce vertex count by up to 90%
class VoxelMesher {
public:
    // Generate mesh from chunk data with greedy meshing optimization
    static Mesh GenerateChunkMesh(const Chunk& chunk, 
                                   const std::unordered_map<long long, Chunk>* neighborChunks = nullptr);
    
private:
    // Enhanced vertex structure with ambient occlusion
    struct Vertex {
        float x, y, z;      // Position
        float nx, ny, nz;   // Normal
        float u, v;         // Texture coordinates
        unsigned char r, g, b, a; // Color + AO
    };
    
    // Greedy meshing for a single plane (XZ, XY, or YZ)
    static void MeshGreedyPlane(const Chunk& chunk,
                                std::vector<Vertex>& vertices,
                                int axis, // 0=XZ, 1=XY, 2=YZ
                                int fixedCoord,
                                bool positive,
                                const std::unordered_map<long long, Chunk>* neighborChunks);
    
    // Add an optimized quad face to the mesh
    static void AddOptimizedFace(std::vector<Vertex>& vertices,
                                float x, float y, float z,
                                Face face,
                                const BlockProperties& props,
                                const std::array<unsigned char, 4>& aoLevels = {255, 255, 255, 255});
    
    // Calculate ambient occlusion for a vertex
    static unsigned char CalculateAO(const Chunk& chunk,
                                   int x, int y, int z,
                                   Face face,
                                   int corner,
                                   const std::unordered_map<long long, Chunk>* neighborChunks);
    
    // Get UV coordinates with proper texture atlas mapping
    static std::pair<float, float> GetUV(int texIndex, float u, float v);
};
