#pragma once
#include "terrain.h"
#include <vector>

class MarchingCubes {
private:
    static const int triTable[256][16];
    static const int edgeTable[256];
    
    Vector3 InterpolateVertex(Vector3 p1, Vector3 p2, float d1, float d2);

public:
    Mesh GenerateMesh(const Chunk& chunk);
};
