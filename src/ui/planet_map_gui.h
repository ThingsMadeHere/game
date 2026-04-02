#pragma once
#include "raylib.h"
#include <vector>
#include <string>

struct PlanetMapEntry {
    std::string name;
    std::string id;
    Vector3 position;
    float radius;           // Display radius (scaled)
    float actualRadius;     // Real radius in Earth radii
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
    
    void DrawPlanet(const PlanetMapEntry& planet);
    void DrawOrbits();
    void DrawUI();
};
