#pragma once
#include "raylib.h"

struct CameraController {
    Camera3D camera;
    float yaw;
    float pitch;

    void Init();
    void Update();
};
