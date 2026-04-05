#include "marching_cubes.h"
#include "../terrain/noise.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>

// Marching Cubes is deprecated - use VoxelMesher instead
// This file is kept for backward compatibility only

const int MarchingCubes::edgeTable[256] = {0};
const int MarchingCubes::triTable[256][16] = {{0}};

Vector3 MarchingCubes::InterpolateVertex(Vector3 p1, Vector3 p2, float d1, float d2) {
    return p1; // Stub implementation
}

Mesh MarchingCubes::GenerateMesh(const Chunk& chunk) {
    // Deprecated - return empty mesh
    // Use VoxelMesher::GenerateChunkMesh instead
    fprintf(stderr, "WARNING: MarchingCubes::GenerateMesh is deprecated. Use VoxelMesher instead.\n");
    return {0};
}
