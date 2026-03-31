#pragma once
#include "../terrain/terrain.h"
#include "raylib.h"
#include <vector>
#include <unordered_map>

class MarchingCubes {
public:
    Mesh GenerateMesh(const Chunk& chunk);
    Mesh GenerateMesh(const Chunk& chunk, const std::unordered_map<ChunkKey, Chunk>& allChunks);
    
private:
    static const int edgeTable[256];
    static const int triTable[256][16];
    
    Vector3 InterpolateVertex(Vector3 p1, Vector3 p2, float d1, float d2);
    float GetDensityWithNeighbors(const Chunk& chunk, const std::unordered_map<ChunkKey, Chunk>& allChunks, int x, int y, int z);
};
