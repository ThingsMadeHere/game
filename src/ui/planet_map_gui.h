#pragma once
#include "raylib.h"
#include <vector>
#include <string>

struct PlanetMapEntry {
    std::string name;
    std::string id;
    Vector3 position;
    Vector3 basePosition;     // Base orbital position (relative to parent)
    float radius;           // Display radius (scaled)
    float actualRadius;     // Real radius in Earth radii
    float semiMajorAxis;    // For orbital period calculation
    float eccentricity;
    float orbitalPeriod;    // Calculated from semi-major axis
    float orbitAngle;       // Current angle in orbit (degrees)
    Color color;
    bool isMoon;
    std::string parentName;
};

class PlanetMapGUI {
public:
    void Init();
    void Update(float deltaTime);
    void Render();
    void Show();
    void Hide();
    bool IsVisible() const { return visible; }
    void LoadPlanetsFromSystem();
    
private:
    bool visible = false;
    std::vector<PlanetMapEntry> planets;
    Camera3D camera = {0};
    float rotationAngle = 0.0f;
    int selectedPlanet = -1;
    Vector3 cameraTarget = {0, 0, 0}; // Camera focus point
    double simulationTime = 0.0;      // Accumulated time for orbits
    float timeScale = 0.5f;           // Speed multiplier for orbit animation
    float aspectRatio = 1.0f;         // Screen aspect ratio for projection matrix
    bool isRightMouseDragging = false; // Right mouse drag state
    Vector2 lastMousePos = {0, 0};    // Last mouse position for drag calculation
    RenderTexture2D sceneTexture = {0}; // Texture for rendering 3D scene
    float cameraYaw = 0.0f;           // Yaw rotation for camera
    float cameraPitch = 0.0f;         // Pitch rotation for camera
    
    void DrawPlanet(const PlanetMapEntry& planet);
    void DrawOrbits();
    void DrawUI();
    void UpdateOrbitalPositions(float deltaTime);
};
