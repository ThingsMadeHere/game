// src/main.cpp
#include "raylib.h"
#include "raymath.h"
#include "Planet.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

const int INTERNAL_WIDTH = 960;
const int INTERNAL_HEIGHT = 540;

int main() {
    // Load config
    std::ifstream configFile("data/config.json");
    json config = json::parse(configFile);
    
    float planetRadius = config.value("planetRadius", 50.0f);
    int chunkResolution = config.value("chunkResolution", 32);
    float gridSize = config.value("gridSize", 0.5f);

    InitWindow(1920, 1080, "Low-Poly Planet Demo");
    SetTargetFPS(60);

    RenderTexture2D gameBuffer = LoadRenderTexture(INTERNAL_WIDTH, INTERNAL_HEIGHT);
    
    // Create sky cubemap textures
    Image skyImages[6];
    Texture2D skyTextures[6];
    
    // Generate cubemap faces with gradient
    for (int face = 0; face < 6; face++) {
        skyImages[face] = GenImageColor(512, 512, BLANK);
        
        for (int y = 0; y < 512; y++) {
            float t = (float)y / 511.0f;
            Color color;
            
            // Different color for top face (face 4)
            if (face == 4) {
                color = {
                    (unsigned char)(100 + t * 155),    // R: 100 to 255
                    (unsigned char)(50 + t * 100),     // G: 50 to 150  
                    (unsigned char)(200 + t * 55),     // B: 200 to 255
                    255
                };
            } else {
                color = {
                    (unsigned char)(0 + t * 135),      // R: 0 to 135
                    (unsigned char)(0 + t * 206),      // G: 0 to 206  
                    (unsigned char)(0 + t * 250),      // B: 0 to 250
                    255
                };
            }
            
            for (int x = 0; x < 512; x++) {
                ImageDrawPixel(&skyImages[face], x, y, color);
            }
        }
        
        // Add white circle to top face (face index 4 or 5 depending on convention)
        if (face == 4) { // Top face
            int centerX = 256;
            int centerY = 256;
            int radius = 80;
            
            for (int y = centerY - radius; y <= centerY + radius; y++) {
                for (int x = centerX - radius; x <= centerX + radius; x++) {
                    int dx = x - centerX;
                    int dy = y - centerY;
                    if (dx*dx + dy*dy <= radius*radius) {
                        if (x >= 0 && x < 512 && y >= 0 && y < 512) {
                            ImageDrawPixel(&skyImages[face], x, y, WHITE);
                        }
                    }
                }
            }
        }
        
        skyTextures[face] = LoadTextureFromImage(skyImages[face]);
    }

    // Create skybox model once
    float skySize = 200.0f;
    Mesh skyMesh = GenMeshCube(skySize, skySize, 10.0f);
    Model skyModel = LoadModelFromMesh(skyMesh);
    skyModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = skyTextures[0]; // Use first face

    Camera3D camera = {0};
    camera.position = (Vector3){0, 25, 25};  // Position to see the chunks at origin
    camera.target = (Vector3){0, 0, 0};     // Look at origin where chunks are
    camera.up = (Vector3){0, 1, 0};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Planet planet(planetRadius, chunkResolution);

    Shader shader = LoadShader("shaders/vertex_snap.glsl", "shaders/fragment_light.glsl");

    Vector3 sunDir = (Vector3){0.5f, -1.0f, 0.3f};
    Vector3 sunColor = (Vector3){1.0f, 0.95f, 0.8f};
    Vector3 ambientColor = (Vector3){0.3f, 0.3f, 0.35f};

    // Create proper light space matrix for shadow mapping
    Vector3 lightPos = {-sunDir.x * 100, -sunDir.y * 100, -sunDir.z * 100};
    Vector3 lightTarget = {0, 0, 0};
    Vector3 lightUp = {0, 1, 0};
    
    Matrix lightView = MatrixLookAt(lightPos, lightTarget, lightUp);
    Matrix lightProj = MatrixOrtho(-100.0f, 100.0f, -100.0f, 100.0f, -100.0f, 100.0f);
    Matrix lightSpaceMatrix = MatrixMultiply(lightView, lightProj);

    // Set shader uniforms (Raylib uses SHADER_UNIFORM_*)
    int shadowMapLoc = GetShaderLocation(shader, "shadowMap");
    int sunDirLoc = GetShaderLocation(shader, "sunDirection");
    int sunColorLoc = GetShaderLocation(shader, "sunColor");
    int ambientLoc = GetShaderLocation(shader, "ambientColor");
    int mvpLoc = GetShaderLocation(shader, "mvp");
    int modelLoc = GetShaderLocation(shader, "model");
    int lightSpaceLoc = GetShaderLocation(shader, "lightSpaceMatrix");
    
    // Set initial light values
    SetShaderValue(shader, sunDirLoc, &sunDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, sunColorLoc, &sunColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, ambientLoc, &ambientColor, SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(shader, lightSpaceLoc, lightSpaceMatrix);

    // Camera rotation variables
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;

    while (!WindowShouldClose()) {
        // Update camera based on input
        Vector3 moveDirection = {0};
        
        // WASD + Q/E movement
        if (IsKeyDown(KEY_W)) moveDirection.z += 1.0f;  // Forward
        if (IsKeyDown(KEY_S)) moveDirection.z -= 1.0f;  // Backward
        if (IsKeyDown(KEY_A)) moveDirection.x -= 1.0f;
        if (IsKeyDown(KEY_D)) moveDirection.x += 1.0f;
        if (IsKeyDown(KEY_Q)) moveDirection.y -= 1.0f; // Down
        if (IsKeyDown(KEY_E)) moveDirection.y += 1.0f; // Up
        
        // Mouse look
        Vector2 mouseDelta = GetMouseDelta();
        cameraYaw -= mouseDelta.x * 0.003f;
        cameraPitch += mouseDelta.y * 0.003f; // Inverted Y axis
        
        // Wrap yaw to prevent trigonometric issues
        while (cameraYaw > PI * 2.0f) cameraYaw -= PI * 2.0f;
        while (cameraYaw < -PI * 2.0f) cameraYaw += PI * 2.0f;
        
        if (cameraPitch > 1.5f) cameraPitch = 1.5f;
        if (cameraPitch < -1.5f) cameraPitch = -1.5f;
        
        // Apply rotation to movement direction manually
        float cosYaw = cosf(cameraYaw);
        float sinYaw = sinf(cameraYaw);
        Vector3 rotatedMove = {
            moveDirection.x * cosYaw - moveDirection.z * sinYaw,
            moveDirection.y,
            moveDirection.x * sinYaw + moveDirection.z * cosYaw
        };
        
        // Update camera position
        float moveSpeed = 0.5f;
        camera.position.x += rotatedMove.x * moveSpeed;
        camera.position.y += rotatedMove.y * moveSpeed;
        camera.position.z += rotatedMove.z * moveSpeed;
        
        // Update camera target based on yaw and pitch
        camera.target.x = camera.position.x + sinf(cameraYaw) * cosf(cameraPitch);
        camera.target.y = camera.position.y + sinf(cameraPitch);
        camera.target.z = camera.position.z + cosf(cameraYaw) * cosf(cameraPitch);
        
        planet.Update(camera.position);

        // RENDER SHADOW PASS
        BeginTextureMode(planet.GetShadowMap());
            ClearBackground(BLANK);
            BeginMode3D(camera);
                // Simplified for demo - draw normally
            EndMode3D();
        EndTextureMode();

        SetShaderValueTexture(shader, shadowMapLoc, planet.GetShadowMap().texture);

        // RENDER MAIN PASS
        BeginTextureMode(gameBuffer);
            // Clear to light blue
            ClearBackground((Color){135, 206, 250, 255});
            
            BeginMode3D(camera);
                // Set shader uniforms once per frame
                Matrix viewProj = GetCameraMatrix(camera);
                SetShaderValueMatrix(shader, mvpLoc, viewProj);
                SetShaderValueMatrix(shader, lightSpaceLoc, lightSpaceMatrix);
                
                // Draw skybox - reuse pre-created model
                Vector3 skyPos = {camera.position.x, 50.0f, camera.position.z}; // Position at Y=50
                DrawModel(skyModel, skyPos, 1.0f, WHITE);
                
                // Draw planet with marching cubes terrain
                planet.Draw(shader);
            EndMode3D();
        EndTextureMode();

        // DRAW TO SCREEN
        BeginDrawing();
            ClearBackground(BLACK);
            
            DrawTexturePro(
                gameBuffer.texture,
                (Rectangle){0, 0, (float)INTERNAL_WIDTH, (float)INTERNAL_HEIGHT},
                (Rectangle){0, 0, 1920, 1080},
                (Vector2){0, 0},
                0,
                WHITE
            );

            DrawFPS(10, 10);
            DrawText("WASD + Q/E + Mouse to move | ESC to quit", 10, 1050, 20, WHITE);
            
            // Display camera position for debugging
            char camPos[100];
            snprintf(camPos, sizeof(camPos), "Camera: (%.1f, %.1f, %.1f)", 
                     camera.position.x, camera.position.y, camera.position.z);
            DrawText(camPos, 10, 40, 20, WHITE);
            
            // Display camera rotation for debugging
            char camRot[100];
            snprintf(camRot, sizeof(camRot), "Pitch: %.3f, Yaw: %.3f", 
                     cameraPitch, cameraYaw);
            DrawText(camRot, 10, 70, 20, WHITE);
        EndDrawing();
    }

    UnloadShader(shader);
    UnloadRenderTexture(gameBuffer);
    
    // Unload skybox model
    UnloadModel(skyModel);
    
    // Unload sky textures
    for (int i = 0; i < 6; i++) {
        UnloadTexture(skyTextures[i]);
        UnloadImage(skyImages[i]);
    }
    
    CloseWindow();

    return 0;
}
