#include "loading_screen.h"
#include <cstdio>
#include <cmath>

void LoadingScreen::Init() {
    isLoading = false;
    chunksGenerated = 0;
    totalChunks = 0;
    chunkOrder.clear();
}

void LoadingScreen::GenerateSpiralOrder(int centerX, int centerZ, int radius) {
    chunkOrder.clear();
    
    // Add center chunk first
    chunkOrder.push_back({centerX, centerZ});
    
    // Generate spiral pattern
    for (int r = 1; r <= radius; r++) {
        // Start at top of the ring
        int x = centerX - r;
        int z = centerZ + r;
        
        // Top edge (left to right)
        for (int i = 0; i < r * 2; i++) {
            chunkOrder.push_back({x, z});
            x++;
        }
        
        // Right edge (top to bottom)
        for (int i = 0; i < r * 2; i++) {
            chunkOrder.push_back({x, z});
            z--;
        }
        
        // Bottom edge (right to left)
        for (int i = 0; i < r * 2; i++) {
            chunkOrder.push_back({x, z});
            x--;
        }
        
        // Left edge (bottom to top)
        for (int i = 0; i < r * 2; i++) {
            chunkOrder.push_back({x, z});
            z++;
        }
    }
    
    totalChunks = chunkOrder.size();
}

void LoadingScreen::StartLoading(int playerChunkX, int playerChunkZ, int radius) {
    isLoading = true;
    chunksGenerated = 0;
    
    GenerateSpiralOrder(playerChunkX, playerChunkZ, radius);
    
    printf("Starting loading: %d chunks in spiral pattern from (%d,%d) radius %d\n", 
           totalChunks.load(), playerChunkX, playerChunkZ, radius);
}

void LoadingScreen::UpdateProgress(int chunksDone) {
    chunksGenerated.store(chunksDone);
}

void LoadingScreen::Render() {
    if (!isLoading) return;
    
    // Full screen dark background
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {20, 20, 30, 255});
    
    // Title
    DrawText("GENERATING WORLD", GetScreenWidth()/2 - 150, 100, 40, WHITE);
    DrawText("Spiral Chunk Generation", GetScreenWidth()/2 - 140, 150, 20, GRAY);
    
    // Progress bar
    int barWidth = 600;
    int barHeight = 30;
    int barX = GetScreenWidth()/2 - barWidth/2;
    int barY = GetScreenHeight()/2 - 15;
    
    // Background bar
    DrawRectangle(barX, barY, barWidth, barHeight, {50, 50, 60, 255});
    
    // Progress fill
    float progress = (float)chunksGenerated / totalChunks;
    int fillWidth = (int)(barWidth * progress);
    DrawRectangle(barX, barY, fillWidth, barHeight, {100, 200, 100, 255});
    
    // Progress border
    DrawRectangleLinesEx({(float)barX, (float)barY, (float)barWidth, (float)barHeight}, 2, WHITE);
    
    // Progress text
    char progressText[64];
    sprintf(progressText, "%d / %d chunks (%.1f%%)", 
            chunksGenerated.load(), totalChunks.load(), progress * 100.0f);
    DrawText(progressText, GetScreenWidth()/2 - 80, barY + 40, 20, WHITE);
    
    // Spiral visualization - calculate radius from chunk count
    int spiralRadius = (int)sqrtf(totalChunks / 4.0f);
    int spiralSize = 200;
    int spiralX = GetScreenWidth() - spiralSize - 50;
    int spiralY = GetScreenHeight() - spiralSize - 50;
    
    // Draw spiral grid
    int cellSize = spiralSize / (spiralRadius * 2 + 1);
    int centerOffset = spiralRadius * cellSize;
    
    // Find center chunk from the order
    int centerX = chunkOrder.empty() ? 0 : chunkOrder[0].first;
    int centerZ = chunkOrder.empty() ? 0 : chunkOrder[0].second;
    
    for (size_t i = 0; i < chunkOrder.size(); i++) {
        auto [cx, cz] = chunkOrder[i];
        
        // Convert chunk coordinates to screen coordinates
        int screenX = spiralX + centerOffset + (cx - centerX) * cellSize;
        int screenY = spiralY + centerOffset + (cz - centerZ) * cellSize;
        
        // Color based on generation status
        Color color = (i < chunksGenerated) ? GREEN : RED;
        
        // Draw chunk
        DrawRectangle(screenX, screenY, cellSize - 1, cellSize - 1, color);
    }
    
    // Draw player position (center)
    DrawRectangle(spiralX + centerOffset - 2, spiralY + centerOffset - 2, 5, 5, YELLOW);
    
    // Instructions
    DrawText("Generating world in spiral pattern from your position...", 
             GetScreenWidth()/2 - 200, GetScreenHeight() - 100, 16, LIGHTGRAY);
}

void LoadingScreen::FinishLoading() {
    isLoading = false;
    printf("Loading complete: %d chunks generated\n", chunksGenerated.load());
}
