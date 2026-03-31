#pragma once
#include "../terrain/terrain.h"
#include "../rendering/marching_cubes.h"
#include "../rendering/gpu_renderer.h"
#include "../rendering/model.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <string>

struct ChunkRequest {
    int cx, cy, cz;
};

class World {
private:
    std::unordered_map<ChunkKey, Chunk> chunks;
    MarchingCubes marchingCubes;
    ModelManager modelManager; // Separate system for .obj models
    
    // Threading for async chunk generation
    std::queue<ChunkRequest> chunkQueue;
    std::mutex queueMutex;
    std::atomic<bool> running;
    std::vector<std::thread> workerThreads;
    
public:
    Chunk& GetChunk(int cx, int cy, int cz);
    Chunk* GetChunkIfExists(int cx, int cy, int cz);
    void GenerateTerrain(Chunk& chunk);
    void GenerateMesh(Chunk& chunk);
    void GenerateAllMeshes(); // Generate meshes for all chunks after terrain is loaded
    void UpdateChunk(int cx, int cy, int cz);
    void SetVoxel(int cx, int cy, int cz, int x, int y, int z, float density);
    float GetVoxel(int cx, int cy, int cz, int x, int y, int z);
    void Render(const Camera3D& camera);
    void RenderShadows(const Matrix& lightSpaceMatrix, Shader& shadowShader);
    void Init();
    void Cleanup();
    
    // Simple save system
    void SaveWorld(const std::string& filename);
    int GetModifiedChunkCount();
    
    // Threading
    void QueueChunkGeneration(int cx, int cy, int cz);
    void ProcessChunkQueue();
    ChunkRequest PopChunkRequest();
    bool IsRunning() { return running.load(); }
    std::mutex& GetMutex() { return queueMutex; }
    std::unordered_map<ChunkKey, Chunk>& GetChunks() { return chunks; }
    MarchingCubes& GetMarchingCubes() { return marchingCubes; }
    
    // Model management (separate from voxel terrain)
    ModelManager& GetModelManager() { return modelManager; }
    void AddModel(const std::string& name, const std::string& objPath, 
                  const std::string& texPath, Vector3 position, 
                  Vector3 rotation = {0,0,0}, float scale = 1.0f);
    void DrawModels(); // Draw all models (called after voxel rendering)
    
    // Garbage collection
    void GarbageCollectChunks(int playerChunkX, int playerChunkZ, int maxDistance);
    int GetGeneratedChunkCount() const;
    
    // Boundary synchronization
    void RegenerateNeighborChunks(int cx, int cy, int cz);
};
