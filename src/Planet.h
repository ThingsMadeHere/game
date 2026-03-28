// src/Planet.h
#pragma once
#include "raylib.h"
#include "MarchingCubes.h"
#include "ThreadPool.h"
#include <vector>
#include <mutex>
#include <functional>

struct Chunk {
    int chunkX, chunkY, chunkZ;
    Mesh mesh = {0};
    bool isDirty = true;
    bool isLoading = false;
    std::mutex mutex;
};

class Planet {
public:
    Planet(float planetRadius, int chunkResolution);
    ~Planet();

    void Update(Vector3 playerPosition);
    void Draw(Shader shader, Matrix lightSpaceMatrix);
    
    // Make shadow map accessible
    RenderTexture2D GetShadowMap() const { return shadowMap; }

private:
    void GenerateChunk(Chunk* chunk);
    float DensityFunction(float x, float y, float z);
    bool IsMeshReady(const Mesh& mesh) const;
    Mesh LoadMeshFromData(const MeshData& data);

    float planetRadius;
    int chunkResolution;
    std::vector<Chunk*> chunks;
    ThreadPool* threadPool;
    
    RenderTexture2D shadowMap;
    const int SHADOW_RES = 2048;
};
