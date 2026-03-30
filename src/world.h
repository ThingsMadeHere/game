#pragma once
#include "terrain.h"
#include "marching_cubes.h"
#include <unordered_map>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>

struct ChunkRequest {
    int cx, cy, cz;
};

class World {
private:
    std::unordered_map<ChunkKey, Chunk> chunks;
    MarchingCubes marchingCubes;
    
    // Threading for async chunk generation
    std::queue<ChunkRequest> chunkQueue;
    std::mutex queueMutex;
    std::atomic<bool> running;
    std::vector<std::thread> workerThreads;
    
public:
    Chunk& GetChunk(int cx, int cy, int cz);
    Chunk* GetChunkIfExists(int cx, int cy, int cz);
    void GenerateTerrain(Chunk& chunk);
    void UpdateChunk(int cx, int cy, int cz);
    void SetVoxel(int cx, int cy, int cz, int x, int y, int z, float density);
    void Render(const Camera3D& camera);
    void Init();
    void Cleanup();
    
    // Threading
    void QueueChunkGeneration(int cx, int cy, int cz);
    void ProcessChunkQueue();
    ChunkRequest PopChunkRequest();
    bool IsRunning() { return running.load(); }
    std::mutex& GetMutex() { return queueMutex; }
    std::unordered_map<ChunkKey, Chunk>& GetChunks() { return chunks; }
    MarchingCubes& GetMarchingCubes() { return marchingCubes; }
    
    // Garbage collection
    void GarbageCollectChunks(int playerChunkX, int playerChunkZ, int maxDistance);
    int GetGeneratedChunkCount() const;
};
