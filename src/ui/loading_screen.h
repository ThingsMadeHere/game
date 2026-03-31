#pragma once
#include "raylib.h"
#include <vector>
#include <atomic>

struct LoadingScreen {
    bool isLoading = false;
    std::atomic<int> chunksGenerated{0};
    std::atomic<int> totalChunks{0};
    std::vector<std::pair<int, int>> chunkOrder; // Spiral order of chunks to generate
    
    void Init();
    void StartLoading(int playerChunkX, int playerChunkZ, int radius);
    void UpdateProgress(int chunksDone);
    void Render();
    bool IsLoading() const { return isLoading; }
    void FinishLoading();
    
private:
    void GenerateSpiralOrder(int centerX, int centerZ, int radius);
};
