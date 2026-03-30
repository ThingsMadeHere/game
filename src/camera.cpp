#include "camera.h"
#include <cstdio>
#include <cmath>

void CameraController::Init() {
    // Start camera at ground level looking at the terrain
    camera.position = {0, 15, 0};
    camera.target = {10, 15, 10};
    camera.up = {0, 1, 0};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    yaw = -45.0f;
    pitch = -25.0f;
    
    printf("Camera initialized: pos(%.1f,%.1f,%.1f) -> target(%.1f,%.1f,%.1f)\n",
           camera.position.x, camera.position.y, camera.position.z,
           camera.target.x, camera.target.y, camera.target.z);
}

void CameraController::Update() {
    float moveSpeed = 0.5f;
    float mouseSensitivity = 0.1f; // Increased from 0.003f

    // Mouse look
    if (IsCursorHidden()) {
        Vector2 mouseDelta = GetMouseDelta();
        yaw -= mouseDelta.x * mouseSensitivity;
        pitch -= mouseDelta.y * mouseSensitivity;
    }

    // Keyboard movement
    if (IsKeyDown(KEY_W)) {
        camera.position.x += sinf(yaw * DEG2RAD) * moveSpeed;
        camera.position.z += cosf(yaw * DEG2RAD) * moveSpeed;
    }
    if (IsKeyDown(KEY_S)) {
        camera.position.x -= sinf(yaw * DEG2RAD) * moveSpeed;
        camera.position.z -= cosf(yaw * DEG2RAD) * moveSpeed;
    }
    if (IsKeyDown(KEY_A)) {
        camera.position.x -= cosf(yaw * DEG2RAD) * moveSpeed;
        camera.position.z += sinf(yaw * DEG2RAD) * moveSpeed;
    }
    if (IsKeyDown(KEY_D)) {
        camera.position.x += cosf(yaw * DEG2RAD) * moveSpeed;
        camera.position.z -= sinf(yaw * DEG2RAD) * moveSpeed;
    }
    if (IsKeyDown(KEY_SPACE)) camera.position.y += moveSpeed;
    if (IsKeyDown(KEY_LEFT_SHIFT)) camera.position.y -= moveSpeed;

    // Fallback keyboard rotation (if mouse doesn't work)
    if (IsKeyDown(KEY_LEFT)) yaw += 1.0f;
    if (IsKeyDown(KEY_RIGHT)) yaw -= 1.0f;
    if (IsKeyDown(KEY_UP)) pitch += 1.0f;
    if (IsKeyDown(KEY_DOWN)) pitch -= 1.0f;

    pitch = fmaxf(-89, fminf(89, pitch));

    // Update camera target based on position and rotation
    float dist = 1.0f; // Shorter distance for better feel
    camera.target.x = camera.position.x + dist * sinf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD);
    camera.target.y = camera.position.y + dist * sinf(pitch * DEG2RAD);
    camera.target.z = camera.position.z + dist * cosf(yaw * DEG2RAD) * cosf(pitch * DEG2RAD);
}
