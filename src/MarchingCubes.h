// src/MarchingCubes.h
#pragma once
#include <vector>
#include <cmath>
#include <functional>

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

class MarchingCubes {
public:
    static MeshData GenerateChunk(
        float centerX, float centerY, float centerZ,
        float chunkSize, int resolution,
        std::function<float(float, float, float)> densityFunc
    );

private:
    static float GetDensity(float x, float y, float z,
                           std::function<float(float, float, float)> densityFunc);
    
    static Vertex InterpolateVertex(const Vertex& v1, const Vertex& v2, float val1, float val2, float isoLevel);
    
    static const int edgeTable[256];
    static const int triTable[256][16];
    
    static void AddTriangle(MeshData& mesh, const Vertex& v1, const Vertex& v2, const Vertex& v3);
    static void CalculateNormal(Vertex& vertex, float x, float y, float z, 
                               std::function<float(float, float, float)> densityFunc, float epsilon = 0.01f);
};
