// src/main.cpp
#include "raylib.h"
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
    
    // Create sky texture manually
    Image skyImage = GenImageColor(512, 512, BLANK);
    for (int y = 0; y < 512; y++) {
        float t = (float)y / 511.0f;
        Color color = {
            (unsigned char)(0 + t * 135),      // R: 0 to 135
            (unsigned char)(0 + t * 206),      // G: 0 to 206  
            (unsigned char)(0 + t * 250),      // B: 0 to 250
            255
        };
        for (int x = 0; x < 512; x++) {
            ImageDrawPixel(&skyImage, x, y, color);
        }
    }
    Texture2D skyTexture = LoadTextureFromImage(skyImage);
    UnloadImage(skyImage);

    Camera3D camera = {0};
    camera.position = (Vector3){0, 20, 60};  // Moved closer and higher
    camera.target = (Vector3){0, 0, 0};
    camera.up = (Vector3){0, 1, 0};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Planet planet(planetRadius, chunkResolution);

    Shader shader = LoadShader("shaders/vertex_snap.glsl", "shaders/fragment_light.glsl");

    Vector3 sunDir = (Vector3){0.5f, -1.0f, 0.3f};
    Vector3 sunColor = (Vector3){1.0f, 0.95f, 0.8f};
    Vector3 ambientColor = (Vector3){0.3f, 0.3f, 0.35f};

    // Create light space matrix (simplified for now)
    Matrix lightSpaceMatrix = { 0 };
    lightSpaceMatrix.m0 = 1.0f;
    lightSpaceMatrix.m5 = 1.0f; 
    lightSpaceMatrix.m10 = 1.0f;
    lightSpaceMatrix.m15 = 1.0f;

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
    SetShaderValue(shader, lightSpaceLoc, &lightSpaceMatrix, SHADER_UNIFORM_FLOAT);

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
                // Set shader uniforms for this frame
                Matrix viewProj = GetCameraMatrix(camera);
                SetShaderValueMatrix(shader, GetShaderLocation(shader, "mvp"), viewProj);
                
                // Draw skybox with texture - using model approach
                float skySize = 200.0f;
                Vector3 skyPos = {camera.position.x, 50.0f, camera.position.z}; // Position at Y=50
                
                // Create and draw skybox model once per frame
                Mesh skyMesh = GenMeshCube(skySize, skySize, 10.0f);
                Model skyModel = LoadModelFromMesh(skyMesh);
                skyModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = skyTexture;
                
                // Draw sky without depth writing
                DrawModel(skyModel, skyPos, 1.0f, WHITE);
                
                // Clean up sky model
                UnloadModel(skyModel);
                
                // Draw sun as a simple cube
                Vector3 sunPos = {-sunDir.x * 50, -sunDir.y * 50, -sunDir.z * 50};
                DrawCube(sunPos, 10.0f, 10.0f, 10.0f, (Color){255, 255, 200, 255});
                
                // Draw planet with marching cubes terrain
                planet.Draw(shader, lightSpaceMatrix);
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
        EndDrawing();
    }

    UnloadShader(shader);
    UnloadRenderTexture(gameBuffer);
    UnloadTexture(skyTexture);
    CloseWindow();

    return 0;
}
