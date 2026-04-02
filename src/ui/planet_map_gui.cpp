#include "planet_map_gui.h"
#include "../core/planet.h"
#include <cmath>
#include <cstdio>

void PlanetMapGUI::Init() {
    camera.position = {50.0f, 50.0f, 50.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    rotationAngle = 0.0f;
    selectedPlanet = -1;
    LoadPlanetsFromSystem();
}

void PlanetMapGUI::LoadPlanetsFromSystem() {
    planets.clear();
    
    auto allIds = g_PlanetSystem.GetAllPlanetIds();
    
    // First pass: create entries for all planets
    for (const auto& id : allIds) {
        const PlanetDefinition* planet = g_PlanetSystem.GetPlanet(id);
        if (!planet) continue;
        
        PlanetMapEntry entry;
        entry.name = planet->name;
        entry.id = planet->id;
        entry.actualRadius = planet->physical.radius; // Real radius in Earth radii
        // Scale radius for display: make star bigger, planets visible, moons small but visible
        if (planet->planetType == "star") {
            entry.radius = planet->physical.radius * 0.5f; // Star is big
        } else {
            entry.radius = 0.5f + planet->physical.radius * 0.3f; // Min 0.5, scale smaller
        }
        
        // Get orbital position (relative to parent)
        // Stars stay at origin, other bodies orbit their parent
        Vector3 pos = {0, 0, 0};
        if (planet->planetType != "star" || !planet->orbit.parentObjectId.empty()) {
            pos = PlanetUtils::CalculateOrbitalPosition(planet->orbit, 0.0f);
        }
        entry.position = {pos.x * 0.05f, pos.y * 0.05f, pos.z * 0.05f}; // Larger scale for visibility
        
        // Color based on planet type
        if (planet->planetType == "star") {
            entry.color = {255, 200, 50, 255}; // Yellow sun
        } else if (planet->planetType == "terrestrial") {
            entry.color = {100, 200, 100, 255}; // Green
        } else if (planet->planetType == "ocean") {
            entry.color = {50, 100, 200, 255}; // Blue
        } else if (planet->planetType == "volcanic") {
            entry.color = {200, 50, 50, 255}; // Red
        } else if (planet->planetType == "ice") {
            entry.color = {200, 220, 255, 255}; // White-blue
        } else if (planet->planetType == "gas_giant") {
            entry.color = {200, 150, 100, 255}; // Orange-brown
        } else {
            entry.color = {150, 150, 150, 255}; // Gray default
        }
        
        entry.isMoon = !planet->orbit.parentObjectId.empty();
        entry.parentName = planet->orbit.parentObjectId;
        
        planets.push_back(entry);
    }
    
    // Second pass: adjust moon/planet positions to be relative to their parents
    // This creates the proper hierarchy: star at center, planets orbit star, moons orbit planets
    for (auto& body : planets) {
        if (!body.isMoon) continue; // Skip stars and root-level planets
        
        // Find parent and add parent's position to this body's position
        for (const auto& parent : planets) {
            if (parent.id == body.parentName) {
                // Add parent's position to this body's position (recursive positioning)
                body.position.x += parent.position.x;
                body.position.y += parent.position.y;
                body.position.z += parent.position.z;
                break;
            }
        }
    }
}

void PlanetMapGUI::Show() {
    visible = true;
    // Reset camera
    camera.position = {50.0f, 50.0f, 50.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
}

void PlanetMapGUI::Hide() {
    visible = false;
}

void PlanetMapGUI::Update(float deltaTime) {
    if (!visible) return;
    
    // Auto-rotate the system
    rotationAngle += 5.0f * deltaTime;
    if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;
    
    // Camera controls
    if (IsKeyDown(KEY_UP)) camera.position.y += 2.0f;
    if (IsKeyDown(KEY_DOWN)) camera.position.y -= 2.0f;
    if (IsKeyDown(KEY_LEFT)) rotationAngle -= 2.0f;
    if (IsKeyDown(KEY_RIGHT)) rotationAngle += 2.0f;
    
    // Zoom
    if (IsKeyDown(KEY_W)) {
        camera.position.x *= 0.98f;
        camera.position.y *= 0.98f;
        camera.position.z *= 0.98f;
    }
    if (IsKeyDown(KEY_S)) {
        camera.position.x *= 1.02f;
        camera.position.y *= 1.02f;
        camera.position.z *= 1.02f;
    }
    
    // Update camera position based on rotation
    float dist = sqrtf(camera.position.x * camera.position.x + camera.position.z * camera.position.z);
    float rad = rotationAngle * (PI / 180.0f);
    camera.position.x = dist * cosf(rad);
    camera.position.z = dist * sinf(rad);
    
    // Planet selection with mouse
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Ray ray = GetMouseRay(GetMousePosition(), camera);
        
        selectedPlanet = -1;
        float closestDist = 999999.0f;
        
        for (size_t i = 0; i < planets.size(); i++) {
            // Simple sphere-ray intersection
            Vector3 toPlanet = {
                planets[i].position.x - ray.position.x,
                planets[i].position.y - ray.position.y,
                planets[i].position.z - ray.position.z
            };
            
            float dist2 = toPlanet.x * toPlanet.x + toPlanet.y * toPlanet.y + toPlanet.z * toPlanet.z;
            float radius2 = planets[i].radius * planets[i].radius;
            
            if (dist2 < radius2 * 4.0f && dist2 < closestDist) {
                closestDist = dist2;
                selectedPlanet = (int)i;
            }
        }
    }
    
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M)) {
        Hide();
    }
}

void PlanetMapGUI::Render() {
    if (!visible) return;
    
    BeginMode3D(camera);
    
    DrawOrbits();
    
    for (const auto& planet : planets) {
        DrawPlanet(planet);
    }
    
    EndMode3D();
    
    DrawUI();
}

void PlanetMapGUI::DrawPlanet(const PlanetMapEntry& planet) {
    // Draw planet sphere
    DrawSphere(planet.position, planet.radius, planet.color);
    
    // Draw selection highlight
    if (selectedPlanet >= 0 && planets[selectedPlanet].id == planet.id) {
        DrawSphereWires(planet.position, planet.radius * 1.2f, 16, 16, YELLOW);
    }
    
    // Draw label if selected or large enough
    if ((selectedPlanet >= 0 && planets[selectedPlanet].id == planet.id) || planet.radius > 2.0f) {
        Vector2 screenPos = GetWorldToScreen(planet.position, camera);
        // Note: Can't draw 2D text here, handled in DrawUI
    }
}

void PlanetMapGUI::DrawOrbits() {
    // Draw orbital paths as circles
    for (const auto& planet : planets) {
        if (!planet.isMoon) continue; // Skip stars/planets without parents
        
        // Find parent position
        Vector3 parentPos = {0, 0, 0};
        for (const auto& p : planets) {
            if (p.id == planet.parentName) {
                parentPos = p.position;
                break;
            }
        }
        
        // Calculate orbit radius
        float orbitRadius = sqrtf(
            (planet.position.x - parentPos.x) * (planet.position.x - parentPos.x) +
            (planet.position.z - parentPos.z) * (planet.position.z - parentPos.z)
        );
        
        // Draw orbit circle
        DrawCircle3D(parentPos, orbitRadius, {1, 0, 0}, 90.0f, {100, 100, 150, 100});
    }
}

void PlanetMapGUI::DrawUI() {
    // Background panel
    DrawRectangle(10, 10, 300, 150, {0, 0, 0, 200});
    DrawRectangleLines(10, 10, 300, 150, WHITE);
    
    // Title
    DrawText("SYSTEM MAP", 20, 20, 24, WHITE);
    DrawText("Press M to close", 20, 50, 16, GRAY);
    DrawText("Arrows/WASD to move", 20, 70, 16, GRAY);
    DrawText("Click to select planet", 20, 90, 16, GRAY);
    
    // Planet count
    char countText[64];
    sprintf(countText, "Planets: %zu", planets.size());
    DrawText(countText, 20, 120, 16, WHITE);
    
    // Selected planet info
    if (selectedPlanet >= 0 && selectedPlanet < (int)planets.size()) {
        const auto& p = planets[selectedPlanet];
        
        DrawRectangle(GetScreenWidth() - 310, 10, 300, 120, {0, 0, 0, 200});
        DrawRectangleLines(GetScreenWidth() - 310, 10, 300, 120, YELLOW);
        
        DrawText("SELECTED", GetScreenWidth() - 300, 20, 20, YELLOW);
        DrawText(p.name.c_str(), GetScreenWidth() - 300, 45, 18, WHITE);
        
        char info[128];
        sprintf(info, "Type: %s", p.isMoon ? "Moon" : "Planet");
        DrawText(info, GetScreenWidth() - 300, 70, 14, GRAY);
        
        sprintf(info, "Radius: %.0f km", p.actualRadius * 6371.0f);
        DrawText(info, GetScreenWidth() - 300, 90, 14, GRAY);
    }
    
    // Planet name labels in 3D space
    for (const auto& planet : planets) {
        Vector2 screenPos = GetWorldToScreen(planet.position, camera);
        
        // Only draw if on screen
        if (screenPos.x > 0 && screenPos.x < GetScreenWidth() &&
            screenPos.y > 0 && screenPos.y < GetScreenHeight()) {
            
            // Check if selected
            bool isSelected = (selectedPlanet >= 0 && planets[selectedPlanet].id == planet.id);
            Color textColor = isSelected ? YELLOW : WHITE;
            
            DrawText(planet.name.c_str(), (int)screenPos.x - 20, (int)screenPos.y - 10, 12, textColor);
        }
    }
}
