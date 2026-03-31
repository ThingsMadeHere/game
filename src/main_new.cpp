#include "raylib.h"
#include "camera.h"
#include "world.h"
#include "voxel_renderer.h"
#include "game_menu.h"
#include "loading_screen.h"
#include "noise.h"
#include <cstdio>
#include <cmath>

// Forward declare functions
void DrawSkyGradient(Camera3D camera);
void InitSkyRenderer();
void RenderSky(Camera3D camera);
void UpdatePlayerPhysics(Camera3D& camera);
bool CheckCollision(Vector3 position);
float GetTerrainHeight(float x, float z);

// Global sky rendering variables
Shader skyShader = {0};
Mesh skyMesh = {0};
Model skyModel = {0};

// Player physics variables
Vector3 playerVelocity = {0.0f, 0.0f, 0.0f};
Vector3 playerPosition = {0.0f, 20.0f, 0.0f}; // Start above ground
bool isGrounded = false;
const float GRAVITY = -9.81f;
const float PLAYER_HEIGHT = 1.8f;
const float JUMP_FORCE = 8.0f;

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
    
    // Initialize sky renderer
    InitSkyRenderer();
    
    // Game state
    bool gameInitialized = false;  // Make sure this starts as false
    MenuState currentMenuState = MenuState::MAIN_MENU;
    bool isLoadingWorld = false;
    
    printf("DEBUG: gameInitialized initialized to: %s\n", gameInitialized ? "TRUE" : "FALSE");
    
    // Initialize systems
    LoadingScreen loadingScreen;
    loadingScreen.Init();

    while (!WindowShouldClose()) {
        currentMenuState = menu.Update();
        printf("DEBUG: Menu state = %d\n", (int)currentMenuState);
        
        if (currentMenuState == MenuState::PLAYING) {
            printf("DEBUG: Entered PLAYING state\n");
            // Handle loading screen
            if (isLoadingWorld) {
                printf("DEBUG: In loading screen\n");
                // Get player position for garbage collection
                Vector3 playerPos = cameraController.camera.position;
                int playerChunkX = (int)floorf(playerPos.x / (CHUNK_SIZE * VOXEL_SIZE));
                int playerChunkZ = (int)floorf(playerPos.z / (CHUNK_SIZE * VOXEL_SIZE));
                
                // Process completed chunks from worker threads
                world.ProcessChunkQueue();
                printf("DEBUG: Processed chunk queue\n");
                
                // Force process some chunks immediately to show progress
                for (int i = 0; i < 10; i++) {
                    world.ProcessChunkQueue();
                }
                printf("DEBUG: Force processed chunks\n");
                
                // Update loading progress
                int chunkCount = world.GetGeneratedChunkCount();
                int totalChunks = loadingScreen.totalChunks;
                printf("DEBUG: Generated chunks: %d, Total needed: %d\n", chunkCount, totalChunks);
                loadingScreen.UpdateProgress(chunkCount);
                
                // Check if loading is complete
                if (chunkCount >= totalChunks) {
                    printf("DEBUG: Loading complete! Setting isLoadingWorld = false\n");
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
                printf("DEBUG: First time game initialization - gameInitialized was false\n");
                gameInitialized = true;
                DisableCursor();
                
                // Start spiral loading
                Vector3 playerPos = cameraController.camera.position;
                int playerChunkX = (int)floorf(playerPos.x / (CHUNK_SIZE * VOXEL_SIZE));
                int playerChunkZ = (int)floorf(playerPos.z / (CHUNK_SIZE * VOXEL_SIZE));
                
                // Generate spiral pattern for loading screen
                // Add center chunk first
                world.QueueChunkGeneration(playerChunkX, 0, playerChunkZ);
                
                for (int r = 1; r <= 8; r++) {
                    // Start at top of the ring
                    int x = playerChunkX - r;
                    int z = playerChunkZ + r;
                    
                    // Top edge (left to right)
                    for (int i = 0; i < r * 2; i++) {
                        world.QueueChunkGeneration(x, 0, z);
                        x++;
                    }
                    
                    // Right edge (top to bottom)
                    for (int i = 0; i < r * 2; i++) {
                        world.QueueChunkGeneration(x, 0, z);
                        z--;
                    }
                    
                    // Bottom edge (right to left)
                    for (int i = 0; i < r * 2; i++) {
                        world.QueueChunkGeneration(x, 0, z);
                        x--;
                    }
                    
                    // Left edge (bottom to top)
                    for (int i = 0; i < r * 2; i++) {
                        world.QueueChunkGeneration(x, 0, z);
                        z++;
                    }
                }
                
                loadingScreen.StartLoading(playerChunkX, playerChunkZ, 2);  // Reduce from 8 to 2 for testing
                isLoadingWorld = true;
                printf("DEBUG: Started loading screen, isLoadingWorld = true\n");
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
                printf("Settings applied: Mouse=%.2f, Render=%d\n", 
                       settings.mouseSensitivity, settings.renderDistance);
            }
            
            // Handle save request
            if (menu.SaveRequested()) {
                world.SaveWorld("world_save.dat");
                printf("DEBUG: World saved!\n");
            }
            
            // Update camera (only when cursor is hidden)
            if (IsCursorHidden()) {
                // Use original camera update - physics is causing freeze
                cameraController.Update();
            }
            
            // Voxel interaction (only when cursor is hidden) - TEMPORARILY DISABLED
            /*
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
            */
            
            BeginDrawing();
            ClearBackground({135, 206, 235, 255}); // Sky blue background
            
            // 3D rendering
            BeginMode3D(cameraController.camera);
            // Draw sky first (behind everything)
            RenderSky(cameraController.camera);
            world.Render(cameraController.camera);
            EndMode3D();
            
            // 2D UI
            DrawFPS(10, 10);
            DrawText("ESC: Menu", 10, 40, 20, RAYWHITE);
            DrawText("Basic Camera Mode - No Physics", 10, 70, 10, YELLOW);
            
            ClearBackground(BLACK);
            menu.Render();
            EndDrawing();
        }
    }
    
    world.Cleanup(); // Cleanup GPU renderer
    CloseWindow();
    return 0;
}

void InitSkyRenderer() {
    // Load shaders
    skyShader = LoadShader("src/sky_vertex.glsl", "src/sky_fragment.glsl");
    
    // Create sky mesh (cube)
    skyMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    skyModel = LoadModelFromMesh(skyMesh);
    skyModel.materials[0].shader = skyShader;
    
    // Set shader uniforms
    Shader skyShaderInst = skyModel.materials[0].shader;
    
    // Sun position (normalized)
    Vector3 sunPos = {0.7f, 0.6f, 0.0f};
    float sunLen = sqrtf(sunPos.x*sunPos.x + sunPos.y*sunPos.y + sunPos.z*sunPos.z);
    sunPos.x /= sunLen; sunPos.y /= sunLen; sunPos.z /= sunLen;
    
    // Set shader uniforms with realistic Earth values
    SetShaderValue(skyShaderInst, GetShaderLocation(skyShaderInst, "uSunPos"), &sunPos, SHADER_UNIFORM_VEC3);
    
    float sunIntensity = 22.0f;
    SetShaderValue(skyShaderInst, GetShaderLocation(skyShaderInst, "uSunIntensity"), &sunIntensity, SHADER_UNIFORM_FLOAT);
    
    float planetRadius = 6371e3f; // Earth radius in meters
    SetShaderValue(skyShaderInst, GetShaderLocation(skyShaderInst, "uPlanetRadius"), &planetRadius, SHADER_UNIFORM_FLOAT);
    
    float atmosphereRadius = 6471e3f; // Atmosphere radius in meters
    SetShaderValue(skyShaderInst, GetShaderLocation(skyShaderInst, "uAtmosphereRadius"), &atmosphereRadius, SHADER_UNIFORM_FLOAT);
    
    // Rayleigh scattering coefficients (wavelength-dependent) - scaled up for visibility
    Vector3 rayleighScattering = {5.5e-3f, 13.0e-3f, 22.4e-3f};
    SetShaderValue(skyShaderInst, GetShaderLocation(skyShaderInst, "uRayleighScattering"), &rayleighScattering, SHADER_UNIFORM_VEC3);
    
    float mieScattering = 21e-3f;
    SetShaderValue(skyShaderInst, GetShaderLocation(skyShaderInst, "uMieScattering"), &mieScattering, SHADER_UNIFORM_FLOAT);
    
    float rayleighScaleHeight = 8e3f; // 8km
    SetShaderValue(skyShaderInst, GetShaderLocation(skyShaderInst, "uRayleighScaleHeight"), &rayleighScaleHeight, SHADER_UNIFORM_FLOAT);
    
    float mieScaleHeight = 1.2e3f; // 1.2km
    SetShaderValue(skyShaderInst, GetShaderLocation(skyShaderInst, "uMieScaleHeight"), &mieScaleHeight, SHADER_UNIFORM_FLOAT);
    
    float mieG = 0.758f; // Mie phase function parameter
    SetShaderValue(skyShaderInst, GetShaderLocation(skyShaderInst, "uMieG"), &mieG, SHADER_UNIFORM_FLOAT);
}

void RenderSky(Camera3D camera) {
    // Draw sky model with shader
    DrawModel(skyModel, camera.position, 1000.0f, WHITE);
}

void UpdatePlayerPhysics(Camera3D& camera) {
    // Ultra-simple physics to prevent freezing
    static float yVelocity = 0.0f;
    static bool grounded = false;
    
    // Simple gravity
    if (!grounded) {
        yVelocity += GRAVITY * 0.016f;
        if (yVelocity < -10.0f) yVelocity = -10.0f;
    }
    
    // Update Y position
    float groundY = 15.0f; // Simple fixed ground for now
    camera.position.y += yVelocity * 0.016f;
    
    // Ground collision
    if (camera.position.y <= groundY) {
        camera.position.y = groundY;
        yVelocity = 0.0f;
        grounded = true;
    } else {
        grounded = false;
    }
    
    // Simple jump
    if (IsKeyPressed(KEY_SPACE) && grounded) {
        yVelocity = 5.0f;
        grounded = false;
    }
    
    // Simple movement (no X/Z for now to isolate issue)
    float moveSpeed = 10.0f;
    if (IsKeyDown(KEY_W)) camera.position.z -= moveSpeed * 0.016f;
    if (IsKeyDown(KEY_S)) camera.position.z += moveSpeed * 0.016f;
    if (IsKeyDown(KEY_A)) camera.position.x -= moveSpeed * 0.016f;
    if (IsKeyDown(KEY_D)) camera.position.x += moveSpeed * 0.016f;
    
    // Update player position for display
    playerPosition = camera.position;
    playerVelocity.y = yVelocity;
    isGrounded = grounded;
}

bool CheckCollision(Vector3 position) {
    // Simple ground collision check
    float terrainHeight = GetTerrainHeight(position.x, position.z);
    return position.y <= terrainHeight + PLAYER_HEIGHT;
}

float GetTerrainHeight(float x, float z) {
    // Sample terrain height using noise function
    float height = 8.0f + SmoothNoise3D(x * 0.02f, 0, z * 0.02f) * 6.0f;
    return height;
}
