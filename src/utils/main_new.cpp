#include "raylib.h"
#include <raymath.h>
#include "../ui/game_menu.h"
#include "../core/world.h"
#include "../gameplay/inventory.h"
#include "../ui/loading_screen.h"
// #include "../audio/audio_system.h"
#include "../terrain/noise.h"
#include "../rendering/deferred_renderer.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <chrono>

// Forward declare functions
void DrawSkyGradient(Camera3D camera);
void InitSkyRenderer();
void RenderSky(Camera3D camera);
void UpdatePlayerPhysics(Camera3D& camera);
struct CollisionInfo {
    bool hit;
    float groundY;
};
CollisionInfo CheckCollision(Vector3 position, World& world);
float GetTerrainHeight(float x, float z);

// Global sky rendering variables
Shader skyShader = {0};
Mesh skyMesh = {0};
Model skyModel = {0};

// Global deferred renderer
DeferredRenderer g_deferredRenderer;

// Player physics variables
Vector3 playerVelocity = {0.0f, 0.0f, 0.0f};
Vector3 playerPosition = {0.0f, 20.0f, 0.0f}; // Start above ground
bool isGrounded = false;
const float GRAVITY = -9.81f;
const float PLAYER_HEIGHT = 1.83f;  // 6 feet tall
const float PLAYER_WIDTH = 0.5f;    // Shoulder width
const float PLAYER_DEPTH = 0.3f;    // Front-to-back
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
    Camera camera;
    camera.position = (Vector3){0.0f, 20.0f, 0.0f};
    camera.target = (Vector3){0.0f, 0.0f, 10.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // Initialize audio system
    // AudioSystem audio;
    // audio.Init();
    
    // Initialize game systems
    World world;
    world.Init(); // Initialize GPU renderer
    
    // Initialize inventory with starter blocks
    Inventory inventory;
    inventory.GiveStarterItems();
    
    // Initialize sky renderer
    InitSkyRenderer();
    
    // Initialize deferred renderer
    g_deferredRenderer.Init(screenWidth, screenHeight);
    
    // Game state
    bool gameInitialized = false;  // Make sure this starts as false
    MenuState currentMenuState = MenuState::MAIN_MENU;
    bool isLoadingWorld = false;
    
    fprintf(stderr,"DEBUG: gameInitialized initialized to: %s\n", gameInitialized ? "TRUE" : "FALSE");
    
    // Initialize systems
    LoadingScreen loadingScreen;
    loadingScreen.Init();

    while (!WindowShouldClose()) {
        currentMenuState = menu.Update();
        fprintf(stderr,"DEBUG: Menu state = %d\n", (int)currentMenuState);
        
        if (currentMenuState == MenuState::PLAYING) {
            fprintf(stderr,"DEBUG: Entered PLAYING state\n");
            // Handle loading screen
            if (isLoadingWorld) {
                fprintf(stderr,"DEBUG: In loading screen\n");
                // Get player position for garbage collection
                Vector3 playerPos = camera.position;
                int playerChunkX = (int)floorf(playerPos.x / (CHUNK_SIZE * VOXEL_SIZE));
                int playerChunkZ = (int)floorf(playerPos.z / (CHUNK_SIZE * VOXEL_SIZE));
                
                // Process completed chunks from worker threads
                world.ProcessChunkQueue();
                fprintf(stderr,"DEBUG: Processed chunk queue\n");
                
                // Force process some chunks immediately to show progress
                for (int i = 0; i < 10; i++) {
                    world.ProcessChunkQueue();
                }
                fprintf(stderr,"DEBUG: Force processed chunks\n");
                
                // Update loading progress
                int chunkCount = world.GetGeneratedChunkCount();
                int totalChunks = loadingScreen.totalChunks;
                fprintf(stderr,"DEBUG: Generated chunks: %d, Total needed: %d\n", chunkCount, totalChunks);
                loadingScreen.UpdateProgress(chunkCount);
                
                // Check if inventory is open - pause game updates when open
                if (inventory.IsOpen()) {
                    if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)) {
                        inventory.Close();
                        EnableCursor(); // Re-enable for menu
                    }
                } else {
                    // Check if loading is complete
                    if (chunkCount >= totalChunks) {
                        fprintf(stderr,"DEBUG: Loading complete! Setting isLoadingWorld = false\n");
                        loadingScreen.FinishLoading();
                        isLoadingWorld = false;
                    }
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
                fprintf(stderr,"DEBUG: First time game initialization - gameInitialized was false\n");
                gameInitialized = true;
                DisableCursor();
                
                // Start spiral loading
                Vector3 playerPos = camera.position;
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
                fprintf(stderr,"DEBUG: Started loading screen, isLoadingWorld = true\n");
                continue; // Skip to loading screen
            }
            
            // Get player position for chunk loading
            Vector3 playerPos = camera.position;
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
                fprintf(stderr,"Settings applied: Mouse=%.2f, Render=%d\n", 
                       settings.mouseSensitivity, settings.renderDistance);
            }
            
            // Handle save request
            if (menu.SaveRequested()) {
                world.SaveWorld("world_save.dat");
                fprintf(stderr,"DEBUG: World saved!\n");
            }
            
            // Update camera (only when cursor is hidden)
            if (IsCursorHidden()) {
                // Apply gravity
                static float yVelocity = 0.0f;
                static bool grounded = false;
                float gravity = -9.8f;
                float dt = GetFrameTime();
                
                // Get terrain height at player position for initial ground check
                float terrainHeight = GetTerrainHeight(camera.position.x, camera.position.z) + PLAYER_HEIGHT;
                
                // Apply gravity
                if (!grounded) {
                    yVelocity += gravity * dt;
                    camera.position.y += yVelocity * dt;
                }
                
                // Ground collision using actual voxel data
                auto collision = CheckCollision(camera.position, world);
                if (collision.hit) {
                    // Place player standing on ground (eye level = ground + player height)
                    camera.position.y = collision.groundY + PLAYER_HEIGHT;
                    yVelocity = 0.0f;
                    grounded = true;
                } else {
                    grounded = false;
                }
                
                // Jump
                if (IsKeyPressed(KEY_SPACE) && grounded) {
                    yVelocity = 8.0f;
                    grounded = false;
                }
                
                // Manual camera control for movement
                float moveSpeed = 8.0f * dt;
                
                // Get camera vectors
                Vector3 forward = Vector3Subtract(camera.target, camera.position);
                forward.y = 0; // Keep movement horizontal
                forward = Vector3Normalize(forward);
                Vector3 right = Vector3CrossProduct(forward, camera.up);
                right = Vector3Normalize(right);
                
                // Movement
                if (IsKeyDown(KEY_W)) {
                    camera.position = Vector3Add(camera.position, Vector3Scale(forward, moveSpeed));
                    camera.target = Vector3Add(camera.target, Vector3Scale(forward, moveSpeed));
                }
                if (IsKeyDown(KEY_S)) {
                    camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, moveSpeed));
                    camera.target = Vector3Subtract(camera.target, Vector3Scale(forward, moveSpeed));
                }
                if (IsKeyDown(KEY_A)) {
                    camera.position = Vector3Subtract(camera.position, Vector3Scale(right, moveSpeed));
                    camera.target = Vector3Subtract(camera.target, Vector3Scale(right, moveSpeed));
                }
                if (IsKeyDown(KEY_D)) {
                    camera.position = Vector3Add(camera.position, Vector3Scale(right, moveSpeed));
                    camera.target = Vector3Add(camera.target, Vector3Scale(right, moveSpeed));
                }
                
                // Hotbar selection with number keys
                for (int i = KEY_ONE; i <= KEY_NINE; i++) {
                    if (IsKeyPressed(i)) {
                        inventory.SetSelectedSlot(i - KEY_ONE);
                    }
                }
                
                // Scroll wheel for hotbar
                float scroll = GetMouseWheelMove();
                if (scroll > 0) {
                    inventory.ScrollSelection(-1);
                } else if (scroll < 0) {
                    inventory.ScrollSelection(1);
                }
                
                // Open inventory with E
                if (IsKeyPressed(KEY_E)) {
                    inventory.ToggleOpen();
                }
                
                // Mouse look
                float mouseSensitivity = 0.003f;
                Vector2 mouseDelta = GetMouseDelta();
                
                // Yaw (rotate around Y axis)
                Matrix yawMatrix = MatrixRotateY(-mouseDelta.x * mouseSensitivity);
                Vector3 viewDir = Vector3Subtract(camera.target, camera.position);
                viewDir = Vector3Transform(viewDir, yawMatrix);
                camera.target = Vector3Add(camera.position, viewDir);
                
                // Pitch (rotate around right axis)
                Vector3 view = Vector3Subtract(camera.target, camera.position);
                right = Vector3CrossProduct(view, camera.up);
                right = Vector3Normalize(right);
                Matrix pitchMatrix = MatrixRotate(right, -mouseDelta.y * mouseSensitivity);
                view = Vector3Transform(view, pitchMatrix);
                camera.target = Vector3Add(camera.position, view);
                
                // Voxel interaction - Block removal (left click)
                static Vector3 highlightPos = {0, 0, 0};
                static bool hasHighlight = false;
                
                // Raycast to find target block for highlighting and removal
                Vector3 rayOrigin = camera.position;
                Vector3 rayDir = Vector3Subtract(camera.target, camera.position);
                rayDir = Vector3Normalize(rayDir);
                
                hasHighlight = false;
                for (float t = 0; t < 50.0f; t += 0.1f) {
                    Vector3 hitPos = Vector3Add(rayOrigin, Vector3Scale(rayDir, t));
                    int vx = (int)floorf(hitPos.x / VOXEL_SIZE);
                    int vy = (int)floorf(hitPos.y / VOXEL_SIZE);
                    int vz = (int)floorf(hitPos.z / VOXEL_SIZE);
                    
                    int cx = (int)floorf((float)vx / CHUNK_SIZE);
                    int cy = (int)floorf((float)vy / CHUNK_HEIGHT);
                    int cz = (int)floorf((float)vz / CHUNK_SIZE);
                    int lx = ((vx % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
                    int ly = ((vy % CHUNK_HEIGHT) + CHUNK_HEIGHT) % CHUNK_HEIGHT;
                    int lz = ((vz % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
                    
                    if (cy == 0 && lx >= 0 && lx < CHUNK_SIZE && ly >= 0 && ly < CHUNK_HEIGHT && lz >= 0 && lz < CHUNK_SIZE) {
                        float density = world.GetVoxel(cx, cy, cz, lx, ly, lz);
                        if (density > 0.0f) {
                            // Found a solid block - set highlight
                            highlightPos.x = vx * VOXEL_SIZE + VOXEL_SIZE * 0.5f;
                            highlightPos.y = vy * VOXEL_SIZE + VOXEL_SIZE * 0.5f;
                            highlightPos.z = vz * VOXEL_SIZE + VOXEL_SIZE * 0.5f;
                            hasHighlight = true;
                            
                            // Remove block on left click
                            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                world.SetVoxel(cx, cy, cz, lx, ly, lz, -1.0f);
                                world.UpdateChunk(cx, cy, cz);
                                hasHighlight = false;
                            }
                            break;
                        }
                    }
                }
                
                // Draw highlight outline
                if (hasHighlight) {
                    DrawCubeWires(highlightPos, VOXEL_SIZE + 0.05f, VOXEL_SIZE + 0.05f, VOXEL_SIZE + 0.05f, WHITE);
                }
                
                // Block placing (right click)
                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                    Vector3 rayOrigin = camera.position;
                    Vector3 rayDir = Vector3Subtract(camera.target, camera.position);
                    rayDir = Vector3Normalize(rayDir);
                    Vector3 lastPos = rayOrigin;
                    
                    for (float t = 0; t < 50.0f; t += 0.5f) {
                        Vector3 hitPos = Vector3Add(rayOrigin, Vector3Scale(rayDir, t));
                        int vx = (int)floorf(hitPos.x / VOXEL_SIZE);
                        int vy = (int)floorf(hitPos.y / VOXEL_SIZE);
                        int vz = (int)floorf(hitPos.z / VOXEL_SIZE);
                        
                        int cx = (int)floorf((float)vx / CHUNK_SIZE);
                        int cy = (int)floorf((float)vy / CHUNK_HEIGHT);
                        int cz = (int)floorf((float)vz / CHUNK_SIZE);
                        int lx = ((vx % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
                        int ly = ((vy % CHUNK_HEIGHT) + CHUNK_HEIGHT) % CHUNK_HEIGHT;
                        int lz = ((vz % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
                        
                        if (cy == 0 && lx >= 0 && lx < CHUNK_SIZE && ly >= 0 && ly < CHUNK_HEIGHT && lz >= 0 && lz < CHUNK_SIZE) {
                            // Check if we hit a solid voxel
                            if (world.GetVoxel(cx, cy, cz, lx, ly, lz) > 0.0f) {
                                // Place block at previous position (the empty space before the hit)
                                int pvx = (int)floorf(lastPos.x / VOXEL_SIZE);
                                int pvy = (int)floorf(lastPos.y / VOXEL_SIZE);
                                int pvz = (int)floorf(lastPos.z / VOXEL_SIZE);
                                int pcx = (int)floorf((float)pvx / CHUNK_SIZE);
                                int pcy = (int)floorf((float)pvy / CHUNK_HEIGHT);
                                int pcz = (int)floorf((float)pvz / CHUNK_SIZE);
                                int plx = ((pvx % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
                                int ply = ((pvy % CHUNK_HEIGHT) + CHUNK_HEIGHT) % CHUNK_HEIGHT;
                                int plz = ((pvz % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
                                
                                if (pcy == 0 && plx >= 0 && plx < CHUNK_SIZE && ply >= 0 && ply < CHUNK_HEIGHT && plz >= 0 && plz < CHUNK_SIZE) {
                                    // Get selected block type from inventory
                                    auto& selectedSlot = inventory.GetSelectedItem();
                                    if (!selectedSlot.IsEmpty() && selectedSlot.type == ItemType::BLOCK) {
                                        world.SetVoxel(pcx, pcy, pcz, plx, ply, plz, 1.0f);
                                        world.UpdateChunk(pcx, pcy, pcz);
                                        inventory.RemoveBlock(inventory.GetSelectedSlot(), 1);
                                    }
                                }
                                break;
                            }
                        }
                        lastPos = hitPos;
                    }
                }
            }
            
            BeginDrawing();
            ClearBackground({135, 206, 235, 255}); // Sky blue background
            
            // === DEFERRED RENDERING PIPELINE WITH SHADOWS ===
            
            // 1. SHADOW PASS - Render to shadow map from light's perspective
            Vector3 lightDir = {0.5f, -1.0f, 0.3f};
            Vector3 sceneCenter = camera.position;
            float sceneRadius = 100.0f;
            g_deferredRenderer.BeginShadowPass(lightDir, sceneCenter, sceneRadius);
            world.RenderShadows(g_deferredRenderer.GetLightSpaceMatrix(), g_deferredRenderer.GetShadowShader());
            g_deferredRenderer.EndShadowPass();
            
            // 2. GEOMETRY PASS - Render to G-Buffer from camera's perspective
            g_deferredRenderer.BeginGeometryPass(camera);
            
            // 3D rendering into G-Buffer
            BeginMode3D(camera);
            
            // DEBUG: Draw a simple test cube to verify rendering works
            DrawCube((Vector3){0.0f, 0.0f, 10.0f}, 2.0f, 2.0f, 2.0f, RED);
            
            world.Render(camera);
            EndMode3D();
            
            g_deferredRenderer.EndGeometryPass();
            
            // 3. COMPOSITE PASS - Combine G-Buffer with shadows and apply lighting
            g_deferredRenderer.BeginCompositePass();
            
            Vector3 lightColor = {1.0f, 1.0f, 0.9f};
            Vector3 ambientColor = {0.3f, 0.35f, 0.4f};
            g_deferredRenderer.CompositeWithShadows(camera, lightDir, lightColor, ambientColor);
            
            g_deferredRenderer.EndCompositePass();
            
            // === END DEFERRED RENDERING PIPELINE ===
            
            // 2D UI (rendered on top)
            
            // 2D UI
            DrawFPS(10, 10);
            DrawText("ESC: Menu", 10, 40, 20, RAYWHITE);
            DrawText("WASD: Move | SPACE: Jump | Mouse: Look", 10, 70, 15, YELLOW);
            DrawText("Left Click: Remove | Right Click: Place | E: Inventory | 1-9: Select", 10, 90, 15, YELLOW);
            
            // Draw crosshair at center of screen
            int screenWidth = GetScreenWidth();
            int screenHeight = GetScreenHeight();
            int crosshairSize = 10;
            int ccx = screenWidth / 2;
            int ccy = screenHeight / 2;
            DrawLine(ccx - crosshairSize, ccy, ccx + crosshairSize, ccy, WHITE);
            DrawLine(ccx, ccy - crosshairSize, ccx, ccy + crosshairSize, WHITE);
            
            // Draw inventory UI
            inventory.DrawHotbar(screenWidth, screenHeight);
            if (inventory.IsOpen()) {
                inventory.DrawInventory(screenWidth, screenHeight);
            }
            
            // Draw selected block name
            auto& selected = inventory.GetSelectedItem();
            if (!selected.IsEmpty() && selected.type == ItemType::BLOCK) {
                DrawText(selected.block.name.c_str(), 10, 110, 15, WHITE);
            }
            
            menu.Render();
            EndDrawing();
        } else {
            // Handle main menu rendering
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
    
    // Simple movement
    float moveSpeed = 10.0f;
    if (IsKeyDown(KEY_W)) camera.position.z -= moveSpeed * 0.016f;
    if (IsKeyDown(KEY_S)) camera.position.z += moveSpeed * 0.016f;
    if (IsKeyDown(KEY_A)) camera.position.x -= moveSpeed * 0.016f;
    if (IsKeyDown(KEY_D)) camera.position.x += moveSpeed * 0.016f;
    
    // Vertical movement (Q = up, E = down)
    if (IsKeyDown(KEY_Q)) camera.position.y += moveSpeed * 0.016f;
    if (IsKeyDown(KEY_E)) camera.position.y -= moveSpeed * 0.016f;
    
    // Update player position for display
    playerPosition = camera.position;
    playerVelocity.y = yVelocity;
    isGrounded = grounded;
}

CollisionInfo CheckCollision(Vector3 position, World& world) {
    // position is eye level, we need to check at feet level
    float feetY = position.y - PLAYER_HEIGHT;
    float halfWidth = PLAYER_WIDTH * 0.5f;
    float halfDepth = PLAYER_DEPTH * 0.5f;
    
    // Check corners of the player's feet
    Vector3 corners[4] = {
        {position.x - halfWidth, feetY, position.z - halfDepth},
        {position.x + halfWidth, feetY, position.z - halfDepth},
        {position.x - halfWidth, feetY, position.z + halfDepth},
        {position.x + halfWidth, feetY, position.z + halfDepth}
    };
    
    float highestGround = -999999.0f;
    bool hitAnything = false;
    
    for (int i = 0; i < 4; i++) {
        // Convert world position to voxel coordinates
        int vx = (int)floorf(corners[i].x / VOXEL_SIZE);
        int vy = (int)floorf(corners[i].y / VOXEL_SIZE);
        int vz = (int)floorf(corners[i].z / VOXEL_SIZE);
        
        int cx = (int)floorf((float)vx / CHUNK_SIZE);
        int cy = (int)floorf((float)vy / CHUNK_HEIGHT);
        int cz = (int)floorf((float)vz / CHUNK_SIZE);
        int lx = ((vx % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
        int ly = ((vy % CHUNK_HEIGHT) + CHUNK_HEIGHT) % CHUNK_HEIGHT;
        int lz = ((vz % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
        
        if (cy == 0 && lx >= 0 && lx < CHUNK_SIZE && ly >= 0 && ly < CHUNK_HEIGHT && lz >= 0 && lz < CHUNK_SIZE) {
            float density = world.GetVoxel(cx, cy, cz, lx, ly, lz);
            if (density > 0.0f) {
                float voxelTop = (vy + 1) * VOXEL_SIZE;
                if (voxelTop > highestGround) {
                    highestGround = voxelTop;
                }
                hitAnything = true;
            }
        }
    }
    
    return {hitAnything, highestGround};
}

float GetTerrainHeightAtPoint(float x, float z) {
    return GetTerrainHeight(x, z);
}

float GetTerrainHeight(float x, float z) {
    // Sample terrain height using noise function
    float height = 8.0f + SmoothNoise3D(x * 0.02f, 0, z * 0.02f) * 6.0f;
    return height;
}
