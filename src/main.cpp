#include "raylib.h"
#include <cstdio>
#include <vector>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <unordered_map>
#include <cstdio>

// Constants
const int CHUNK_SIZE = 16;
const int CHUNK_HEIGHT = 32;
const int RENDER_DISTANCE = 3;
const float VOXEL_SIZE = 0.5f; // Decreased from 1.0f for better visualization

// Improved Perlin-like noise implementation
float Hash(float x, float y, float z) {
    // Better hash function for smoother noise
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    int zi = (int)floorf(z) & 255;
    
    // Simple permutation table simulation
    int hash = (xi * 374761393 + yi * 668265263 + zi * 217460973) % 256;
    return (float)hash / 256.0f;
}

float SmoothNoise3D(float x, float y, float z) {
    // Get integer coordinates
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    int iz = (int)floorf(z);
    
    // Get fractional coordinates
    float fx = x - ix;
    float fy = y - iy;
    float fz = z - iz;
    
    // Get hash values for corners
    float h000 = Hash(ix, iy, iz);
    float h001 = Hash(ix, iy, iz + 1);
    float h010 = Hash(ix, iy + 1, iz);
    float h011 = Hash(ix, iy + 1, iz + 1);
    float h100 = Hash(ix + 1, iy, iz);
    float h101 = Hash(ix + 1, iy, iz + 1);
    float h110 = Hash(ix + 1, iy + 1, iz);
    float h111 = Hash(ix + 1, iy + 1, iz + 1);
    
    // Smoothstep interpolation
    float u = fx * fx * (3.0f - 2.0f * fx);
    float v = fy * fy * (3.0f - 2.0f * fy);
    float w = fz * fz * (3.0f - 2.0f * fz);
    
    // Trilinear interpolation
    float x00 = h000 * (1.0f - u) + h100 * u;
    float x01 = h001 * (1.0f - u) + h101 * u;
    float x10 = h010 * (1.0f - u) + h110 * u;
    float x11 = h011 * (1.0f - u) + h111 * u;
    
    float y0 = x00 * (1.0f - v) + x10 * v;
    float y1 = x01 * (1.0f - v) + x11 * v;
    
    return y0 * (1.0f - w) + y1 * w;
}

float FBM(float x, float y, float z, int octaves) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 0.05f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        value += SmoothNoise3D(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return value / maxValue;
}

// Voxel data
struct Voxel {
    float density;
    bool isActive;
};

class Chunk {
public:
    int cx, cy, cz;
    std::vector<Voxel> voxels;
    Mesh mesh;
    bool needsUpdate;

    Chunk() : cx(0), cy(0), cz(0), needsUpdate(true) {
        voxels.resize(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE);
    }

    Chunk(int x, int y, int z) : cx(x), cy(y), cz(z), needsUpdate(true) {
        voxels.resize(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE);
    }

    int GetIndex(int x, int y, int z) const {
        return (y * CHUNK_SIZE * CHUNK_SIZE) + (z * CHUNK_SIZE) + x;
    }

    float GetDensity(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
            return 1.0f;
        }
        return voxels[GetIndex(x, y, z)].density;
    }

    void SetDensity(int x, int y, int z, float d) {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
            voxels[GetIndex(x, y, z)].density = d;
            voxels[GetIndex(x, y, z)].isActive = true;
        }
    }
};

// Marching Cubes tables
const int edgeTable[256] = {
    0x0, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x99, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x33, 0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1a3, 0xaa, 0x7a6, 0x6af, 0x5a5, 0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
    0x460, 0x569, 0x663, 0x76a, 0x66, 0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff, 0x3f5, 0x2fc, 0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55, 0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc, 0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc, 0xcc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x55, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0xff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f, 0x66, 0x76a, 0x663, 0x569, 0x460,
    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa, 0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33, 0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99, 0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};

const int triTable[256][16] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
    {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
    {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
    {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
    {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
    {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
    {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
    {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
    {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
    {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
    {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
    {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
    {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
    {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
    {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
    {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {5, 4, 8, 5, 8, 11, 5, 11, 10, 11, 8, 9, -1, -1, -1, -1},
    {9, 5, 4, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 0, 1, 3, 0, 3, 7, 7, 3, 4, -1, -1, -1, -1},
    {0, 5, 4, 0, 1, 5, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 3, 4, 3, 1, 3, 7, 5, 1, 3, 5, -1, -1, -1, -1},
    {9, 5, 4, 1, 2, 10, 7, 8, 4, 8, 4, 7, -1, -1, -1, -1},
    {1, 2, 10, 4, 7, 8, 4, 8, 0, 0, 8, 3, 0, 3, 9, -1},
    {10, 2, 5, 10, 5, 4, 4, 5, 0, 4, 0, 7, 7, 0, 8, -1},
    {10, 2, 5, 10, 5, 4, 4, 5, 3, 4, 3, 7, 3, 5, 2, -1},
    {9, 5, 4, 2, 3, 11, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
    {2, 0, 11, 2, 11, 5, 5, 11, 4, 4, 11, 7, 7, 11, 9, -1},
    {0, 1, 5, 0, 5, 11, 11, 5, 4, 4, 11, 2, 2, 11, 3, -1},
    {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, 11, 8, 9, -1},
    {10, 3, 11, 10, 1, 3, 9, 5, 4, 4, 7, 8, -1, -1, -1, -1},
    {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 1, 6, 11, 10, 1, -1},
    {0, 5, 4, 0, 11, 5, 0, 3, 11, 11, 5, 4, 4, 5, 7, -1},
    {11, 10, 5, 7, 8, 4, 10, 11, 4, 4, 11, 9, -1, -1, -1, -1},
    {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
    {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
    {6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 5, 6, 7, 7, 6, 3, 3, 6, 2, -1, -1, -1, -1},
    {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, 1, 5, 10, -1},
    {6, 5, 9, 6, 9, 11, 11, 9, 8, 3, 6, 2, 10, 6, 3},
    {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
    {2, 0, 1, 6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1},
    {8, 11, 6, 8, 6, 5, 5, 6, 2, 2, 6, 0, -1, -1, -1, -1},
    {11, 6, 3, 11, 3, 2, 9, 8, 5, 5, 8, 0, -1, -1, -1, -1},
    {3, 11, 6, 3, 6, 5, 5, 6, 7, 7, 6, 10, 10, 6, 1, -1},
    {6, 3, 11, 6, 5, 3, 5, 1, 3, 1, 5, 0, 0, 5, 7, -1},
    {8, 0, 2, 8, 2, 11, 11, 2, 6, 6, 2, 5, 5, 2, 7, -1},
    {11, 6, 3, 11, 3, 2, 9, 8, 0, 0, 8, 5, 5, 8, 7, -1},
    {6, 5, 10, 7, 5, 6, 8, 4, 9, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 4, 7, 8, 4, 3, 7, 3, 0, 7, -1, -1, -1, -1},
    {0, 1, 5, 0, 5, 10, 0, 10, 6, 10, 5, 7, 7, 10, 4, -1},
    {10, 6, 5, 1, 5, 10, 5, 6, 7, 7, 6, 3, 3, 6, 4, -1},
    {6, 5, 10, 6, 7, 5, 7, 4, 5, 2, 3, 11, -1, -1, -1, -1},
    {11, 2, 3, 4, 7, 8, 1, 0, 5, 5, 0, 6, 6, 0, 10, -1},
    {7, 5, 6, 7, 6, 4, 4, 6, 0, 0, 6, 2, 2, 6, 11, -1},
    {3, 11, 2, 7, 5, 6, 8, 4, 9, 9, 4, 0, -1, -1, -1, -1},
    {7, 5, 10, 7, 10, 6, 8, 4, 3, 3, 4, 11, 11, 4, 1, -1},
    {9, 4, 8, 5, 10, 6, 1, 0, 3, 3, 0, 6, 6, 0, 11, -1},
    {6, 5, 10, 6, 7, 5, 7, 4, 5, 4, 5, 0, 0, 5, 11, -1},
    {8, 4, 7, 9, 8, 7, 9, 7, 11, 9, 11, 2, 2, 11, 10, -1},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, -1, -1, -1, -1}
};

class MarchingCubes {
public:
    Mesh GenerateMesh(Chunk& chunk) {
        Mesh mesh = { 0 };
        std::vector<Vector3> vertices;
        std::vector<Vector3> normals;

        int cubesProcessed = 0;
        int cubesWithSurface = 0;

        for (int z = 0; z < CHUNK_SIZE - 1; z++) {
            for (int y = 0; y < CHUNK_HEIGHT - 1; y++) {
                for (int x = 0; x < CHUNK_SIZE - 1; x++) {
                    float density[8];
                    Vector3 pos[8];

                    pos[0] = {(float)x, (float)y, (float)z};
                    pos[1] = {(float)x + 1, (float)y, (float)z};
                    pos[2] = {(float)x, (float)y, (float)z + 1};
                    pos[3] = {(float)x + 1, (float)y, (float)z + 1};
                    pos[4] = {(float)x, (float)y + 1, (float)z};
                    pos[5] = {(float)x + 1, (float)y + 1, (float)z};
                    pos[6] = {(float)x, (float)y + 1, (float)z + 1};
                    pos[7] = {(float)x + 1, (float)y + 1, (float)z + 1};

                    density[0] = chunk.GetDensity(x, y, z);
                    density[1] = chunk.GetDensity(x + 1, y, z);
                    density[2] = chunk.GetDensity(x, y, z + 1);
                    density[3] = chunk.GetDensity(x + 1, y, z + 1);
                    density[4] = chunk.GetDensity(x, y + 1, z);
                    density[5] = chunk.GetDensity(x + 1, y + 1, z);
                    density[6] = chunk.GetDensity(x, y + 1, z + 1);
                    density[7] = chunk.GetDensity(x + 1, y + 1, z + 1);

                    cubesProcessed++;

                    int cubeIndex = 0;
                    for (int i = 0; i < 8; i++) {
                        if (density[i] > 0) cubeIndex |= (1 << i);
                    }

                    if (cubeIndex == 0 || cubeIndex == 255) continue;
                    
                    cubesWithSurface++;

                    const int* tri = triTable[cubeIndex];
                    for (int i = 0; tri[i] != -1 && i < 15; i += 3) {
                        int e1 = tri[i];
                        int e2 = tri[i + 1];
                        int e3 = tri[i + 2];
                        if (e1 < 0 || e1 > 7 || e2 < 0 || e2 > 7 || e3 < 0 || e3 > 7) continue;
                        Vector3 v1 = InterpolateVertex(pos[tri[i]], pos[tri[i + 1]], density[tri[i]], density[tri[i + 1]]);
                        Vector3 v2 = InterpolateVertex(pos[tri[i + 1]], pos[tri[i + 2]], density[tri[i + 1]], density[tri[i + 2]]);
                        Vector3 v3 = InterpolateVertex(pos[tri[i + 2]], pos[tri[i]], density[tri[i + 2]], density[tri[i]]);

                        vertices.push_back(v1);
                        vertices.push_back(v2);
                        vertices.push_back(v3);

                        Vector3 edge1 = {v2.x - v1.x, v2.y - v1.y, v2.z - v1.z};
                        Vector3 edge2 = {v3.x - v1.x, v3.y - v1.y, v3.z - v1.z};
                        Vector3 normal = {
                            edge1.y * edge2.z - edge1.z * edge2.y,
                            edge1.z * edge2.x - edge1.x * edge2.z,
                            edge1.x * edge2.y - edge1.y * edge2.x
                        };
                        float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                        if (len > 0) {
                            normal.x /= len; normal.y /= len; normal.z /= len;
                        }
                        normals.push_back(normal);
                        normals.push_back(normal);
                        normals.push_back(normal);
                    }
                }
            }
        }

        if (vertices.empty()) return {0};

        // Create proper mesh with correct structure
        Mesh resultMesh = {0};
        resultMesh.vertexCount = vertices.size() / 3;
        resultMesh.triangleCount = vertices.size() / 9; // 3 vertices per triangle
                
        // Allocate and copy vertex data
        resultMesh.vertices = (float*)malloc(vertices.size() * sizeof(float));
        resultMesh.normals = (float*)malloc(normals.size() * sizeof(float));
        resultMesh.indices = (unsigned short*)malloc(resultMesh.triangleCount * 3 * sizeof(unsigned short));
                
        memcpy(resultMesh.vertices, vertices.data(), vertices.size() * sizeof(float));
        memcpy(resultMesh.normals, normals.data(), normals.size() * sizeof(float));
                
        // Create sequential indices
        for (int i = 0; i < resultMesh.triangleCount * 3; i++) {
            resultMesh.indices[i] = i;
        }
                
        resultMesh.triangleCount = vertices.size() / 3;
        
        UploadMesh(&resultMesh, false);
        
        // Debug output
        printf("Chunk mesh generated: %d cubes processed, %d with surface, %d vertices\n", 
               cubesProcessed, cubesWithSurface, (int)vertices.size());
        
        return resultMesh;
    }

private:
    Vector3 InterpolateVertex(Vector3 p1, Vector3 p2, float d1, float d2) {
        float t = d1 / (d1 - d2);
        if (t < 0) t = 0;
        if (t > 1) t = 1;
        return {p1.x + t * (p2.x - p1.x), p1.y + t * (p2.y - p1.y), p1.z + t * (p2.z - p1.z)};
    }
};

class World {
private:
    std::unordered_map<int64_t, Chunk> chunks;
    MarchingCubes marchingCubes;

    int64_t GetChunkKey(int x, int y, int z) {
        return ((int64_t)x << 40) | ((int64_t)y << 20) | (int64_t)z;
    }

public:
    Chunk& GetChunk(int x, int y, int z) {
        int64_t key = GetChunkKey(x, y, z);
        auto it = chunks.find(key);
        if (it == chunks.end()) {
            Chunk chunk(x, y, z);
            GenerateTerrain(chunk);
            auto result = chunks.emplace(key, chunk);
            return result.first->second;
        }
        return it->second;
    }

    void GenerateTerrain(Chunk& chunk) {
        printf("Generating terrain for chunk (%d, %d, %d)\n", chunk.cx, chunk.cy, chunk.cz);
        
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    float worldX = (float)(chunk.cx * CHUNK_SIZE + x) * VOXEL_SIZE;
                    float worldY = (float)(chunk.cy * CHUNK_HEIGHT + y) * VOXEL_SIZE;
                    float worldZ = (float)(chunk.cz * CHUNK_SIZE + z) * VOXEL_SIZE;
                    
                    // Generate proper continuous terrain height
                    float groundHeight = 8.0f + SmoothNoise3D(worldX * 0.02f, 0, worldZ * 0.02f) * 6.0f;
                    
                    // Create solid ground below height, air above
                    float density = 0.0f;
                    
                    if (worldY < groundHeight) {
                        density = 1.0f; // Solid ground
                        
                        // Add some caves - hollow out areas below surface
                        if (worldY < groundHeight - 2.0f) {
                            float caveNoise = SmoothNoise3D(worldX * 0.05f, worldY * 0.05f, worldZ * 0.05f);
                            if (caveNoise > 0.6f) {
                                density = -1.0f; // Cave
                            }
                        }
                    } else {
                        density = -1.0f; // Air
                    }
                    
                    chunk.SetDensity(x, y, z, density);
                }
            }
        }
        
        chunk.mesh = marchingCubes.GenerateMesh(chunk);
    }

    void SetVoxel(int cx, int cy, int cz, int x, int y, int z, float density) {
        Chunk& chunk = GetChunk(cx, cy, cz);
        chunk.SetDensity(x, y, z, density);
        chunk.needsUpdate = true;
    }

    void UpdateChunk(int cx, int cy, int cz) {
        int64_t key = GetChunkKey(cx, cy, cz);
        auto it = chunks.find(key);
        if (it != chunks.end()) {
            if (it->second.needsUpdate) {
                UnloadMesh(it->second.mesh);
                it->second.mesh = {0};
                it->second.mesh = marchingCubes.GenerateMesh(it->second);
                it->second.needsUpdate = false;
            }
        }
    }

    void Render(const Camera3D& camera) {
        for (auto& [key, chunk] : chunks) {
            if (chunk.mesh.vertexCount > 0) {
                Matrix transform = {
                    1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    (float)chunk.cx * CHUNK_SIZE, (float)chunk.cy * CHUNK_HEIGHT, (float)chunk.cz * CHUNK_SIZE, 1
                };
                
                // Render terrain as individual voxels (cubes) with smaller size
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    for (int y = 0; y < CHUNK_HEIGHT; y++) {
                        for (int z = 0; z < CHUNK_SIZE; z++) {
                            if (chunk.GetDensity(x, y, z) > 0.0f) {
                                Vector3 pos = {
                                    (float)chunk.cx * CHUNK_SIZE + x * VOXEL_SIZE,
                                    (float)chunk.cy * CHUNK_HEIGHT + y * VOXEL_SIZE,
                                    (float)chunk.cz * CHUNK_SIZE + z * VOXEL_SIZE
                                };
                                // Draw smaller cubes for each voxel
                                DrawCube(pos, VOXEL_SIZE * 0.9f, VOXEL_SIZE * 0.9f, VOXEL_SIZE * 0.9f, {34, 139, 34, 255});
                            }
                        }
                    }
                }
                
                printf("Rendering chunk as voxels: %d voxels\n", 
                       chunk.mesh.vertexCount);
            }
        }
        
        // Draw chunk boundaries
        for (int cx = -RENDER_DISTANCE; cx <= RENDER_DISTANCE; cx++) {
            for (int cz = -RENDER_DISTANCE; cz <= RENDER_DISTANCE; cz++) {
                Vector3 chunkPos = {(float)cx * CHUNK_SIZE, 0, (float)cz * CHUNK_SIZE};
                Vector3 size = {CHUNK_SIZE, 1, CHUNK_SIZE};
                DrawCubeWires(chunkPos, size.x, size.y, size.z, RED);
            }
        }
    }
};

struct CameraController {
    Camera3D camera;
    float yaw;
    float pitch;

    void Init() {
        // Start camera at ground level looking at the terrain
        camera.position = {0, 15, 0};
        camera.target = {10, 15, 10};
        camera.up = {0, 1, 0};
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        yaw = -45.0f;
        pitch = -25.0f;
        
        printf("Camera initialized: pos(%.1f,%.1f,%.1f) -> target(%.1f,%.1f,%.1f)\n",
               camera.position.x, camera.position.y, camera.position.z,
               camera.target.x, camera.target.y, camera.target.z);
    }

    void Update() {
        float moveSpeed = 0.5f;
        float mouseSensitivity = 0.3f;

        // Mouse look
        Vector2 mouseDelta = GetMouseDelta();
        yaw -= mouseDelta.x * mouseSensitivity;
        pitch -= mouseDelta.y * mouseSensitivity;

        // Clamp pitch to prevent over-rotation
        pitch = fmaxf(-89, fminf(89, pitch));

        // Movement controls
        if (IsKeyDown(KEY_W)) {
            camera.position.x += sinf(yaw * DEG2RAD) * moveSpeed;
            camera.position.z += cosf(yaw * DEG2RAD) * moveSpeed;
        }
        if (IsKeyDown(KEY_S)) {
            camera.position.x -= sinf(yaw * DEG2RAD) * moveSpeed;
            camera.position.z -= cosf(yaw * DEG2RAD) * moveSpeed;
        }
        if (IsKeyDown(KEY_D)) {
            camera.position.x -= cosf(yaw * DEG2RAD) * moveSpeed;
            camera.position.z += sinf(yaw * DEG2RAD) * moveSpeed;
        }
        if (IsKeyDown(KEY_A)) {
            camera.position.x += cosf(yaw * DEG2RAD) * moveSpeed;
            camera.position.z -= sinf(yaw * DEG2RAD) * moveSpeed;
        }
        if (IsKeyDown(KEY_SPACE)) camera.position.y += moveSpeed;
        if (IsKeyDown(KEY_LEFT_SHIFT)) camera.position.y -= moveSpeed;

        // Alternative rotation with arrow keys (for fine control)
        float rotSpeed = 0.3f;
        if (IsKeyDown(KEY_LEFT)) yaw += rotSpeed;
        if (IsKeyDown(KEY_RIGHT)) yaw -= rotSpeed;
        if (IsKeyDown(KEY_UP)) pitch += rotSpeed;
        if (IsKeyDown(KEY_DOWN)) pitch -= rotSpeed;

        // Update camera target based on new rotation
        float dist = 20.0f;
        camera.target.x = camera.position.x + dist * sinf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD);
        camera.target.y = camera.position.y + dist * sinf(pitch * DEG2RAD);
        camera.target.z = camera.position.z + dist * cosf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD);
    }
};

void DrawSkyGradient(const Camera3D& camera) {
    // Calculate gradient colors based on world height
    Color topColor = {135, 206, 235, 255};    // Sky blue
    Color bottomColor = {20, 20, 30, 255};    // Dark blue
    
    // Draw a large hemisphere around the camera
    float radius = 200.0f;
    int segments = 32;  // Reduced for better performance
    int rings = 12;     // Reduced for better performance
    
    // Draw sky dome using triangles
    for (int ring = 0; ring < rings; ring++) {
        float phi1 = PI * (float)ring / (float)rings / 2.0f; // Only top hemisphere
        float phi2 = PI * (float)(ring + 1) / (float)rings / 2.0f;
        
        for (int seg = 0; seg < segments; seg++) {
            float theta1 = 2.0f * PI * (float)seg / (float)segments;
            float theta2 = 2.0f * PI * (float)(seg + 1) / (float)segments;
            
            // Calculate vertices for hemisphere
            Vector3 v1 = {
                camera.position.x + radius * sinf(phi1) * cosf(theta1),
                camera.position.y + radius * cosf(phi1),
                camera.position.z + radius * sinf(phi1) * sinf(theta1)
            };
            Vector3 v2 = {
                camera.position.x + radius * sinf(phi1) * cosf(theta2),
                camera.position.y + radius * cosf(phi1),
                camera.position.z + radius * sinf(phi1) * sinf(theta2)
            };
            Vector3 v3 = {
                camera.position.x + radius * sinf(phi2) * cosf(theta2),
                camera.position.y + radius * cosf(phi2),
                camera.position.z + radius * sinf(phi2) * sinf(theta2)
            };
            Vector3 v4 = {
                camera.position.x + radius * sinf(phi2) * cosf(theta1),
                camera.position.y + radius * cosf(phi2),
                camera.position.z + radius * sinf(phi2) * sinf(theta1)
            };
            
            // Calculate color based on height (from top to horizon)
            float heightFactor = 1.0f - (phi1 / (PI / 2.0f));
            heightFactor = heightFactor > 1.0f ? 1.0f : (heightFactor < 0.0f ? 0.0f : heightFactor);
            
            Color color = {
                (unsigned char)(topColor.r * heightFactor + bottomColor.r * (1.0f - heightFactor)),
                (unsigned char)(topColor.g * heightFactor + bottomColor.g * (1.0f - heightFactor)),
                (unsigned char)(topColor.b * heightFactor + bottomColor.b * (1.0f - heightFactor)),
                255
            };
            
            // Draw quad as two triangles
            if (ring == 0) {
                DrawTriangle3D(v1, v2, v3, color);
            } else {
                DrawTriangle3D(v1, v2, v3, color);
                DrawTriangle3D(v1, v3, v4, color);
            }
        }
    }
}

int main() {
    const int screenWidth = 1200;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "3D Voxel Terrain with Marching Cubes");
    SetTargetFPS(60);
    
    // Configure mouse for borderless operation
    DisableCursor();
    SetMousePosition(screenWidth / 2, screenHeight / 2);

    CameraController cam;
    cam.Init();

    World world;

    for (int cx = -RENDER_DISTANCE; cx <= RENDER_DISTANCE; cx++) {
        for (int cz = -RENDER_DISTANCE; cz <= RENDER_DISTANCE; cz++) {
            world.GetChunk(cx, 0, cz);
        }
    }

    while (!WindowShouldClose()) {
        // Toggle cursor with ESC key
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (IsCursorHidden()) {
                EnableCursor();
            } else {
                DisableCursor();
                SetMousePosition(screenWidth / 2, screenHeight / 2);
            }
        }
        
        // Only update camera when cursor is hidden (game mode)
        if (IsCursorHidden()) {
            cam.Update();
        }

        // Only allow voxel interaction when cursor is hidden (game mode)
        if (IsCursorHidden()) {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector3 rayOrigin = cam.camera.position;
            Vector3 rayDir = {
                cam.camera.target.x - cam.camera.position.x,
                cam.camera.target.y - cam.camera.position.y,
                cam.camera.target.z - cam.camera.position.z
            };
            float len = sqrtf(rayDir.x*rayDir.x + rayDir.y*rayDir.y + rayDir.z*rayDir.z);
            rayDir.x /= len; rayDir.y /= len; rayDir.z /= len;

            for (float t = 0; t < 50.0f; t += 0.5f) {
                int vx = (int)(rayOrigin.x + rayDir.x * t);
                int vy = (int)(rayOrigin.y + rayDir.y * t);
                int vz = (int)(rayOrigin.z + rayDir.z * t);

                int cx = vx / CHUNK_SIZE;
                int cy = vy / CHUNK_HEIGHT;
                int cz = vz / CHUNK_SIZE;
                int x = ((vx % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
                int y = ((vy % CHUNK_HEIGHT) + CHUNK_HEIGHT) % CHUNK_HEIGHT;
                int z = ((vz % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

                if (cy == 0 && x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
                    world.SetVoxel(cx, cy, cz, x, y, z, -1.0f);
                    world.UpdateChunk(cx, cy, cz);
                    break;
                }
            }
            }

            if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            Vector3 rayOrigin = cam.camera.position;
            Vector3 rayDir = {
                cam.camera.target.x - cam.camera.position.x,
                cam.camera.target.y - cam.camera.position.y,
                cam.camera.target.z - cam.camera.position.z
            };
            float len = sqrtf(rayDir.x*rayDir.x + rayDir.y*rayDir.y + rayDir.z*rayDir.z);
            rayDir.x /= len; rayDir.y /= len; rayDir.z /= len;

            for (float t = 0; t < 50.0f; t += 0.5f) {
                int vx = (int)(rayOrigin.x + rayDir.x * t);
                int vy = (int)(rayOrigin.y + rayDir.y * t);
                int vz = (int)(rayOrigin.z + rayDir.z * t);

                int cx = vx / CHUNK_SIZE;
                int cy = vy / CHUNK_HEIGHT;
                int cz = vz / CHUNK_SIZE;
                int x = ((vx % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
                int y = ((vy % CHUNK_HEIGHT) + CHUNK_HEIGHT) % CHUNK_HEIGHT;
                int z = ((vz % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

                if (cy == 0 && x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
                    world.SetVoxel(cx, cy, cz, x, y, z, 1.0f);
                    world.UpdateChunk(cx, cy, cz);
                    break;
                }
            }
            }
        } // End of cursor hidden check

        BeginDrawing();
        ClearBackground(BLACK);
        
        // Test: Just draw UI text to verify 2D rendering works
        DrawText("UI TEST - Should be visible", 10, 10, 20, WHITE);
        DrawFPS(10, 50);

        BeginMode3D(cam.camera);

        // Test: Basic rendering verification
        DrawText("3D RENDERING TEST", 10, 10, 20, GREEN);

        // Test: Draw a simple triangle to verify rendering works
        Vector3 v1 = {0, 22, 0};
        Vector3 v2 = {5, 22, 0};
        Vector3 v3 = {2.5f, 27, 0};
        DrawTriangle3D(v1, v2, v3, YELLOW);
        
        // Test: Draw a simple quad to verify mesh rendering works
        Mesh testMesh = { 0 };
        testMesh.vertexCount = 4;
        testMesh.triangleCount = 2;
        
        float* testVertices = (float*)malloc(sizeof(float) * 3 * 4);
        unsigned short* testIndices = (unsigned short*)malloc(sizeof(unsigned short) * 6);
        
        // Create quad vertices
        testVertices[0] = 10; testVertices[1] = 22; testVertices[2] = 0;
        testVertices[3] = 15; testVertices[4] = 22; testVertices[5] = 0;
        testVertices[6] = 15; testVertices[7] = 25; testVertices[8] = 0;
        testVertices[9] = 10; testVertices[10] = 24; testVertices[11] = 0;
        
        // Create quad indices (0,1,2, 0,2,3, 1,3)
        testIndices[0] = 0; testIndices[1] = 1; testIndices[2] = 2;
        testIndices[3] = 0; testIndices[4] = 2; testIndices[5] = 3;
        
        testMesh.vertices = testVertices;
        testMesh.indices = testIndices;
        
        // Initialize mesh properties
        testMesh.texcoords = NULL;
        testMesh.texcoords2 = NULL;
        testMesh.colors = NULL;
        testMesh.tangents = NULL;
        testMesh.boneIds = NULL;
        testMesh.boneWeights = NULL;
        testMesh.animVertices = NULL;
        testMesh.animNormals = NULL;
        testMesh.vaoId = 0;
        testMesh.vboId = NULL;
        
        UploadMesh(&testMesh, false);
        
        // Draw test mesh
        Matrix testTransform = {1,0,0,0, 0,1,0,0, 0,0,1, 5,25,0,1};

        EndMode3D();

        BeginDrawing();
        ClearBackground(BLACK);
        
        // Test: Draw 2D text first to verify basic rendering
        DrawText("2D TEST - Should be visible", 10, 10, 20, WHITE);
        DrawFPS(10, 50);

        BeginMode3D(cam.camera);

        // Draw world space sky gradient
        DrawSkyGradient(cam.camera);

        world.Render(cam.camera);

        EndMode3D();

        // Draw UI elements after 3D rendering
        DrawText("3D Voxel Terrain - Marching Cubes", 10, 10, 20, RAYWHITE);
        DrawText("WASD: Move | Mouse: Look | Space/Shift: Up/Down", 10, 40, 20, RAYWHITE);
        DrawText("Left Click: Destroy | Right Click: Build | ESC: Toggle Cursor", 10, 70, 20, RAYWHITE);
        DrawFPS(10, 100);
        
        // Draw center dot for GUI reference
        DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, 3, RED);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
