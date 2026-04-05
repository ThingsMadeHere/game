#pragma once
#include "../terrain/terrain.h"
#include <vector>

// Fast voxel mesh generator using greedy face culling
// Generates actual block faces instead of marching cubes isosurfaces
class VoxelMesher {
public:
    // Generate mesh from chunk data with face culling
    static Mesh GenerateChunkMesh(const Chunk& chunk, 
                                   const std::unordered_map<long long, Chunk>* neighborChunks = nullptr);
    
private:
    // Vertex structure for block meshes
    struct Vertex {
        float x, y, z;      // Position
        float nx, ny, nz;   // Normal
        float u, v;         // Texture coordinates
        unsigned char r, g, b, a; // Color (for ambient occlusion)
    };
    
    // Add a quad face to the mesh
    static void AddFace(std::vector<Vertex>& vertices, 
                       float x, float y, float z,
                       Face face,
                       const BlockProperties& props,
                       const std::unordered_map<long long, Chunk>* neighborChunks,
                       int cx, int cy, int cz);
};
