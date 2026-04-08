#pragma once
#include "../terrain/terrain.h"
#include <vector>
#include <cstdint>

// Simple voxel mesher - one quad per visible face
class VoxelMesher {
public:
    // Generate mesh from chunk - simple naive approach that always works
    static Mesh GenerateChunkMesh(const Chunk& chunk, 
                                   const std::unordered_map<long long, Chunk>* neighborChunks = nullptr);
    
private:
    struct Vertex {
        float x, y, z;
        float nx, ny, nz;
        float u, v;
        uint8_t cr, cg, cb, ca;
        Vertex(float x_, float y_, float z_, float nx_, float ny_, float nz_, 
               float u_, float v_, uint8_t cr_, uint8_t cg_, uint8_t cb_, uint8_t ca_)
            : x(x_), y(y_), z(z_), nx(nx_), ny(ny_), nz(nz_), u(u_), v(v_), 
              cr(cr_), cg(cg_), cb(cb_), ca(ca_) {}
    };
    
    static void AddFace(std::vector<Vertex>& verts, int x, int y, int z, BlockType type, Face face);
    static void GetTextureUV(BlockType type, Face face, float& u, float& v, float& cellSize);
    static bool ShouldRenderFace(const Chunk& chunk, int x, int y, int z, Face face,
                                  const std::unordered_map<long long, Chunk>* neighbors);
};
