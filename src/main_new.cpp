#include "raylib.h"
#include "camera.h"
#include "world.h"
#include "voxel_renderer.h"
#include "game_menu.h"
#include "loading_screen.h"
#include <cstdio>
#include <cmath>

// Forward declare functions
void DrawSkyGradient(Camera3D camera);

int main() {
    const int screenWidth = 1200;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "Voxel Terrain - Ultra Fast");
    SetTargetFPS(60);
    
    // Start with cursor visible for menu
    EnableCursor();
    
    // Initialize menu
    GameMenu menu;
    menu.Init();
    
    // Initialize camera
    CameraController cameraController;
    cameraController.Init();
    
    // Initialize game systems
    World world;
    world.Init(); // Initialize GPU renderer
    VoxelRenderer::Init(); // Initialize voxel renderer
    
    // Game state
    bool gameInitialized = false;
    MenuState currentMenuState = MenuState::MAIN_MENU;
    bool isLoadingWorld = false;
    
    // Initialize systems
    LoadingScreen loadingScreen;
    loadingScreen.Init();

    while (!WindowShouldClose()) {
        currentMenuState = menu.Update();
        
        if (currentMenuState == MenuState::PLAYING) {
            // Handle loading screen
            if (isLoadingWorld) {
                // Update loading progress
                loadingScreen.UpdateProgress(world.GetGeneratedChunkCount());
                
                // Check if loading is complete
                if (world.GetGeneratedChunkCount() >= loadingScreen.totalChunks) {
                    loadingScreen.FinishLoading();
                    isLoadingWorld = false;
                }
                
                // Render loading screen
                BeginDrawing();
                ClearBackground(BLACK);
                loadingScreen.Render();
                EndDrawing();
                continue; // Skip the rest of the game loop while loading
            }
            
            // Only disable cursor when actually entering game for first time
            if (!gameInitialized) {
                gameInitialized = true;
                DisableCursor();
                
                // Start spiral loading
                Vector3 playerPos = cameraController.camera.position;
                int playerChunkX = (int)floorf(playerPos.x / (CHUNK_SIZE * VOXEL_SIZE));
                int playerChunkZ = (int)floorf(playerPos.z / (CHUNK_SIZE * VOXEL_SIZE));
                
                loadingScreen.StartLoading(playerChunkX, playerChunkZ, 8); // 8 chunk radius
                world.QueueSpiralGeneration(playerChunkX, playerChunkZ, 8);
                isLoadingWorld = true;
                continue; // Skip to loading screen
            }
            
            // Get player position for chunk loading
            Vector3 playerPos = cameraController.camera.position;
            int playerChunkX = (int)floorf(playerPos.x / (CHUNK_SIZE * VOXEL_SIZE));
            int playerChunkZ = (int)floorf(playerPos.z / (CHUNK_SIZE * VOXEL_SIZE));
            
            // Only queue chunks within reasonable distance for performance
            int maxGenerateDistance = 12; // Limit generation to 12 chunks
            
            // Queue chunks around player for async generation (infinite procedural generation)
            for (int x = playerChunkX - maxGenerateDistance; x <= playerChunkX + maxGenerateDistance; x++) {
                for (int z = playerChunkZ - maxGenerateDistance; z <= playerChunkZ + maxGenerateDistance; z++) {
                    // Skip chunks that are too far from player
                    int distX = abs(x - playerChunkX);
                    int distZ = abs(z - playerChunkZ);
                    if (distX > maxGenerateDistance || distZ > maxGenerateDistance) continue;
                    
                    world.QueueChunkGeneration(x, 0, z);
                }
            }
            
            // Process completed chunks
            world.ProcessChunkQueue();
            
            // Apply settings if changed
            if (menu.SettingsWereChanged()) {
                Settings settings = menu.GetSettings();
                // Apply settings to game systems here
                printf("Settings applied: Mouse=%.2f, Render=%d, Voxel=%.2f\n", 
                       settings.mouseSensitivity, settings.renderDistance, settings.voxelSize);
            }
            
            // Update camera (only when cursor is hidden)
            if (IsCursorHidden()) {
                cameraController.Update();
            }
            
            // Voxel interaction (only when cursor is hidden)
            if (IsCursorHidden()) {
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    Vector3 rayOrigin = cameraController.camera.position;
                    Vector3 rayDir = {
                        cameraController.camera.target.x - cameraController.camera.position.x,
                        cameraController.camera.target.y - cameraController.camera.position.y,
                        cameraController.camera.target.z - cameraController.camera.position.z
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
                    Vector3 rayOrigin = cameraController.camera.position;
                    Vector3 rayDir = {
                        cameraController.camera.target.x - cameraController.camera.position.x,
                        cameraController.camera.target.y - cameraController.camera.position.y,
                        cameraController.camera.target.z - cameraController.camera.position.z
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
            }
            
            BeginDrawing();
            ClearBackground(BLACK);
            
            // 3D rendering
            BeginMode3D(cameraController.camera);
            world.Render(cameraController.camera);
            DrawSkyGradient(cameraController.camera);
            EndMode3D();
            
            // 2D UI
            DrawFPS(10, 10);
            DrawText("ESC: Menu", 10, 40, 20, RAYWHITE);
            
            EndDrawing();
            
        } else {
            // Menu is active - menu handles cursor state
            BeginDrawing();
            ClearBackground(BLACK);
            menu.Render();
            EndDrawing();
        }
    }
    
    world.Cleanup(); // Cleanup GPU renderer
    CloseWindow();
    return 0;
}

void DrawSkyGradient(Camera3D camera) {
    // Simple sky gradient - draw a large box around the camera
    Color skyColor = {135, 206, 235, 255}; // Sky blue
    
    // Draw a simple sky box
    DrawCube(camera.position, 1000.0f, 1000.0f, 1000.0f, skyColor);
}
