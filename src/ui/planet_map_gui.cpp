#include "planet_map_gui.h"
#include "../core/planet.h"
#include <cmath>
#include <cstdio>
#include "raylib.h"
#include "rlgl.h"

void PlanetMapGUI::Init() {
    camera.position = {200.0f, 200.0f, 200.0f}; // Increased from 50 for better draw distance
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // Set far clipping plane to a very large distance (effectively remove it)
    rlSetProjectionMatrixPerspective(camera.fovy, (float)GetScreenWidth()/(float)GetScreenHeight(), 0.1f, 100000.0f);
    
    rotationAngle = 0.0f;
    selectedPlanet = -1;
    LoadPlanetsFromSystem();
}

void PlanetMapGUI::LoadPlanetsFromSystem() {
    planets.clear();
    
    auto allIds = g_PlanetSystem.GetAllPlanetIds();
    
    // First pass: create entries for all planets
    for (const auto& id : allIds) {
        // Skip solar system planets - only show Sandos system
        if (id == "earth" || id == "mars" || id == "moon") continue;
        
        const PlanetDefinition* planet = g_PlanetSystem.GetPlanet(id);
        if (!planet) continue;
        
        PlanetMapEntry entry;
        entry.name = planet->name;
        entry.id = planet->id;
        entry.actualRadius = planet->physical.radius; // Real radius in Earth radii
        
        // Use real sizes proportionally
        // Scale factor: 1 Earth radius = 2 units in visualization
        // Star gets special treatment since it's much larger than planets
        float scaleFactor = 2.0f;
        if (planet->planetType == "star") {
            // Star radius scaled down to fit view but still much larger than planets
            // Sandos has radius 0.7 (solar radii), which is ~76x Earth radius
            // Display it as ~40 units to show it's huge but fit in view
            entry.radius = 40.0f * planet->physical.radius;
        } else {
            // Planets and moons use their actual radius (in Earth radii) scaled uniformly
            entry.radius = planet->physical.radius * scaleFactor;
        }
        
        // Get orbital position (relative to parent)
        // Stars stay at origin, other bodies orbit their parent
        Vector3 pos = {0, 0, 0};
        if (planet->planetType != "star" && !planet->orbit.parentObjectId.empty()) {
            pos = PlanetUtils::CalculateOrbitalPosition(planet->orbit, 0.0f);
        }
        // Store orbital parameters for animation
        entry.semiMajorAxis = planet->orbit.semiMajorAxis;
        entry.eccentricity = planet->orbit.eccentricity;
        entry.orbitAngle = 0.0f;
        
        // Calculate orbital period using Kepler's 3rd law: T² ∝ a³
        // For visualization, we scale so closer objects orbit faster
        // Inner planets: fast orbit, Outer planets: slow orbit
        float a = entry.semiMajorAxis;
        if (a > 0) {
            // T = k * a^(3/2) where k is chosen for good visualization
            // Inner planets (a~100): ~5 seconds per orbit
            // Outer planets (a~1800): ~20 seconds per orbit
            float k = 0.15f; 
            entry.orbitalPeriod = k * powf(a, 1.5f);
        } else {
            entry.orbitalPeriod = 0.0f;
        }
        
        // Determine if this is a moon (orbits a planet) or a planet (orbits a star)
        entry.isMoon = false;
        entry.parentName = planet->orbit.parentObjectId;
        float orbitScale = 0.5f; // Default scale for planets
        
        if (!entry.parentName.empty()) {
            const PlanetDefinition* parent = g_PlanetSystem.GetPlanet(entry.parentName);
            if (parent && parent->planetType != "star") {
                entry.isMoon = true; // Only objects orbiting non-stars are moons
                orbitScale = 0.1f; // Much smaller scale for moon orbits
            }
            // If parent is a star or doesn't exist, isMoon stays false (it's a planet)
        }
        
        entry.basePosition = {pos.x * orbitScale, pos.y * orbitScale, pos.z * orbitScale}; // Store base orbit position
        entry.position = entry.basePosition; // Current animated position
        
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
        
        planets.push_back(entry);
    }
    
    // Second pass: adjust planet positions to be relative to their parents (star)
    // Moons keep their own orbit relative to their parent planet (don't add parent's position)
    // This creates the proper hierarchy: star at center, planets orbit star, moons orbit planets
    for (auto& body : planets) {
        if (body.parentName.empty()) continue; // Skip objects without parents (stars)
        
        // Only add parent's position for planets (not moons)
        // Moons should orbit their parent planet using only their own SMA
        if (!body.isMoon) {
            // Find parent and add parent's base position to this body's base position
            for (const auto& parent : planets) {
                if (parent.id == body.parentName) {
                    // Add parent's base position to this body's base position
                    body.basePosition.x += parent.basePosition.x;
                    body.basePosition.y += parent.basePosition.y;
                    body.basePosition.z += parent.basePosition.z;
                    break;
                }
            }
        }
        // Moons keep their basePosition as-is (relative to their parent planet)
    }
    
    // Initialize current positions
    for (auto& body : planets) {
        body.position = body.basePosition;
    }
}

void PlanetMapGUI::Show() {
    visible = true;
    // Reset camera with increased distance
    camera.position = {200.0f, 200.0f, 200.0f}; // Increased from 50 for better draw distance
    camera.target = {0.0f, 0.0f, 0.0f};
}

void PlanetMapGUI::Hide() {
    visible = false;
}

void PlanetMapGUI::Update(float deltaTime) {
    if (!visible) return;
    
    // Update orbital positions based on time
    simulationTime += deltaTime * timeScale;
    UpdateOrbitalPositions(deltaTime);
    
    // Auto-rotate the camera view
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
    
    // Update camera position based on rotation around target
    float dist = sqrtf(
        (camera.position.x - cameraTarget.x) * (camera.position.x - cameraTarget.x) +
        (camera.position.z - cameraTarget.z) * (camera.position.z - cameraTarget.z)
    );
    float rad = rotationAngle * (PI / 180.0f);
    camera.position.x = cameraTarget.x + dist * cosf(rad);
    camera.position.z = cameraTarget.z + dist * sinf(rad);
    camera.target = cameraTarget;
    
    // Planet selection with mouse
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Ray ray = GetMouseRay(GetMousePosition(), camera);
        
        selectedPlanet = -1;
        float closestT = 999999.0f;
        
        for (size_t i = 0; i < planets.size(); i++) {
            // Ray-sphere intersection
            Vector3 L = {
                planets[i].position.x - ray.position.x,
                planets[i].position.y - ray.position.y,
                planets[i].position.z - ray.position.z
            };
            
            float tca = L.x * ray.direction.x + L.y * ray.direction.y + L.z * ray.direction.z;
            if (tca < 0) continue; // Sphere is behind ray origin
            
            float d2 = (L.x * L.x + L.y * L.y + L.z * L.z) - tca * tca;
            float radius2 = planets[i].radius * planets[i].radius;
            
            if (d2 > radius2) continue; // Ray misses sphere
            
            float thc = sqrtf(radius2 - d2);
            float t = tca - thc; // Distance to intersection point
            
            if (t < closestT) {
                closestT = t;
                selectedPlanet = (int)i;
                // Center camera on selected planet
                cameraTarget = planets[i].position;
            }
        }
    }
    
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M)) {
        Hide();
    }
    
    // Press C to reset camera focus to center (star)
    if (IsKeyPressed(KEY_C)) {
        cameraTarget = {0, 0, 0};
        selectedPlanet = -1;
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
    // Draw orbital paths as circles for all orbiting bodies
    for (const auto& planet : planets) {
        // Skip stars (they don't orbit anything)
        if (planet.parentName.empty()) continue;
        
        // Find parent position
        Vector3 parentPos = {0, 0, 0};
        for (const auto& p : planets) {
            if (p.id == planet.parentName) {
                parentPos = p.position;
                break;
            }
        }
        
        // Use base position for consistent orbit radius (not current moving position)
        float orbitRadius = sqrtf(
            planet.basePosition.x * planet.basePosition.x + 
            planet.basePosition.z * planet.basePosition.z
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
    DrawText("Press M to close | C to center on star", 20, 50, 16, GRAY);
    DrawText("Arrows/WASD to move", 20, 70, 16, GRAY);
    DrawText("Click to select planet", 20, 90, 16, GRAY);
    
    // Planet count
    char countText[64];
    sprintf(countText, "Planets: %zu", planets.size());
    DrawText(countText, 20, 120, 16, WHITE);
    
    // Selected planet info
    if (selectedPlanet >= 0 && selectedPlanet < (int)planets.size()) {
        const auto& p = planets[selectedPlanet];
        
        DrawRectangle(GetScreenWidth() - 310, 10, 300, 150, {0, 0, 0, 200});
        DrawRectangleLines(GetScreenWidth() - 310, 10, 300, 150, YELLOW);
        
        DrawText("SELECTED", GetScreenWidth() - 300, 20, 20, YELLOW);
        DrawText(p.name.c_str(), GetScreenWidth() - 300, 45, 18, WHITE);
        
        char info[128];
        // Show correct type: star, planet, or moon
        std::string typeStr = "Planet";
        if (p.id == "sandos") {
            typeStr = "Star";
        } else if (p.isMoon) {
            typeStr = "Moon";
        }
        sprintf(info, "Type: %s", typeStr.c_str());
        DrawText(info, GetScreenWidth() - 300, 70, 14, GRAY);
        
        sprintf(info, "Radius: %.2f Earth radii", p.actualRadius);
        DrawText(info, GetScreenWidth() - 300, 90, 14, GRAY);
        
        sprintf(info, "(~%.0f km)", p.actualRadius * 6371.0f);
        DrawText(info, GetScreenWidth() - 300, 110, 12, GRAY);
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

void PlanetMapGUI::UpdateOrbitalPositions(float deltaTime) {
    // Update each body's position based on its orbital period
    // Stars don't move (they're at center or have no orbit)
    for (auto& body : planets) {
        if (body.orbitalPeriod <= 0.0f) continue; // Skip stars and stationary objects
        
        // Calculate current angle based on time
        // Full circle (360 degrees) in orbitalPeriod seconds
        float angle = (simulationTime / body.orbitalPeriod) * 360.0f;
        body.orbitAngle = fmodf(angle, 360.0f);
        
        // Calculate new position in orbit (relative to parent)
        float rad = body.orbitAngle * (PI / 180.0f);
        
        // For a circular orbit (simplified):
        // Get the base orbital radius from basePosition (distance from parent)
        float orbitRadius = sqrtf(
            body.basePosition.x * body.basePosition.x + 
            body.basePosition.z * body.basePosition.z
        );
        
        // Calculate new relative position
        Vector3 relativePos = {
            orbitRadius * cosf(rad),
            body.basePosition.y, // Keep original Y (inclination)
            orbitRadius * sinf(rad)
        };
        
        // Add parent's current position to get absolute position (only for non-moons)
        // Moons orbit their parent planet, so we need to add the parent's position
        // Planets orbit the star, so we add the star's position (which is at origin)
        if (!body.parentName.empty()) {
            for (const auto& parent : planets) {
                if (parent.id == body.parentName) {
                    // For moons: add parent planet's position to moon's relative orbit
                    // For planets: add star's position (0,0,0) to planet's relative orbit
                    body.position.x = parent.position.x + relativePos.x;
                    body.position.y = parent.position.y + relativePos.y;
                    body.position.z = parent.position.z + relativePos.z;
                    break;
                }
            }
        } else {
            // No parent (shouldn't happen for orbiting bodies), use relative as absolute
            body.position = relativePos;
        }
    }
}
