#include "planet_map_gui.h"
#include "../core/planet.h"
#include <cmath>
#include <cstdio>
#include "raylib.h"
#include "rlgl.h"

void PlanetMapGUI::Init() {
    camera.position = {1000.0f, 1000.0f, 1000.0f}; // Even larger distance for far plane
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // Initialize render texture for 3D scene
    sceneTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    
    // Initialize custom projection matrix for far plane
    aspectRatio = (float)GetScreenWidth() / (float)GetScreenHeight();
    
    rotationAngle = 0.0f;
    selectedPlanet = -1;
    isRightMouseDragging = false;
    lastMousePos = {0, 0};
    cameraYaw = -45.0f;  // Start at 45 degrees yaw
    cameraPitch = -30.0f; // Start at 30 degrees pitch
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
        
        // Use smaller sizes to prevent overlapping
        // Scale factor: 1 Earth radius = 0.8 units in visualization (reduced from 2.0)
        // Star gets special treatment since it's much larger than planets
        float scaleFactor = 0.8f;
        if (planet->planetType == "star") {
            // Star radius scaled down significantly to fit view but still larger than planets
            // Sandos has radius 0.7 (solar radii), which is ~76x Earth radius
            // Display it as ~15 units to show it's larger but not overwhelming
            entry.radius = 15.0f;
        } else {
            // Planets and moons use their actual radius (in Earth radii) scaled uniformly
            entry.radius = planet->physical.radius * scaleFactor;
            
            // Cap maximum radius for planets to prevent overlapping
            if (entry.radius > 3.0f) {
                entry.radius = 3.0f;
            }
            
            // Moons should be even smaller
            if (planet->planetType != "star" && !planet->orbit.parentObjectId.empty()) {
                const PlanetDefinition* parent = g_PlanetSystem.GetPlanet(planet->orbit.parentObjectId);
                if (parent && parent->planetType != "star") {
                    entry.radius *= 0.5f; // Moons are half the size of planets
                    if (entry.radius > 1.5f) {
                        entry.radius = 1.5f; // Cap moon size
                    }
                }
            }
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
        // Inner planets: very fast orbit, Outer planets: still fast orbit
        float a = entry.semiMajorAxis;
        if (a > 0) {
            // T = k * a^(3/2) where k is chosen for good visualization
            // Inner planets (a~100): ~1 second per orbit
            // Outer planets (a~1800): ~4 seconds per orbit
            float k = 0.003f; // Much faster orbital periods (reduced from 0.15f)
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
    // Reset camera with increased distance for better far plane
    camera.position = {1000.0f, 1000.0f, 1000.0f}; // Even larger distance
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
    
    // Calculate camera distance from target
    float cameraDistance = sqrtf(
        (camera.position.x - cameraTarget.x) * (camera.position.x - cameraTarget.x) +
        (camera.position.y - cameraTarget.y) * (camera.position.y - cameraTarget.y) +
        (camera.position.z - cameraTarget.z) * (camera.position.z - cameraTarget.z)
    );
    
    // Mouse wheel zoom
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0) {
        cameraDistance -= wheelMove * 100.0f; // Zoom speed
        if (cameraDistance < 100.0f) cameraDistance = 100.0f;  // Min zoom
        if (cameraDistance > 5000.0f) cameraDistance = 5000.0f; // Max zoom
    }
    
    // Right mouse drag rotation around selected object
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        isRightMouseDragging = true;
        lastMousePos = GetMousePosition();
    }
    
    if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
        isRightMouseDragging = false;
    }
    
    if (isRightMouseDragging) {
        Vector2 currentMousePos = GetMousePosition();
        Vector2 mouseDelta = {
            currentMousePos.x - lastMousePos.x,
            currentMousePos.y - lastMousePos.y
        };
        
        // Update yaw and pitch based on mouse movement
        cameraYaw += mouseDelta.x * 0.5f;        // Horizontal rotation
        cameraPitch -= mouseDelta.y * 0.3f;       // Vertical rotation (inverted for natural feel)
        
        // Clamp pitch to prevent flipping over
        if (cameraPitch > 89.0f) cameraPitch = 89.0f;
        if (cameraPitch < -89.0f) cameraPitch = -89.0f;
        
        lastMousePos = currentMousePos;
    }
    
    // Auto-rotation disabled - no cinematic rotation
    // Camera only rotates with manual user input (right-click drag or keyboard)
    
    // Keyboard controls (when not right-click dragging)
    if (!isRightMouseDragging) {
        // Arrow keys for manual rotation when no planet selected
        if (selectedPlanet < 0) {
            if (IsKeyDown(KEY_LEFT)) cameraYaw -= 2.0f;
            if (IsKeyDown(KEY_RIGHT)) cameraYaw += 2.0f;
        }
        
        // Up/Down arrows for pitch adjustment
        if (IsKeyDown(KEY_UP)) cameraPitch += 2.0f;
        if (IsKeyDown(KEY_DOWN)) cameraPitch -= 2.0f;
        
        // Clamp pitch
        if (cameraPitch > 89.0f) cameraPitch = 89.0f;
        if (cameraPitch < -89.0f) cameraPitch = -89.0f;
        
        // W/S for zoom
        if (IsKeyDown(KEY_W)) {
            cameraDistance *= 0.98f;
            if (cameraDistance < 100.0f) cameraDistance = 100.0f;
        }
        if (IsKeyDown(KEY_S)) {
            cameraDistance *= 1.02f;
            if (cameraDistance > 5000.0f) cameraDistance = 5000.0f;
        }
    }
    
    // Update camera target to selected planet or center
    if (selectedPlanet >= 0 && selectedPlanet < (int)planets.size()) {
        cameraTarget = planets[selectedPlanet].position;
    } else {
        cameraTarget = {0.0f, 0.0f, 0.0f}; // Default to center when no planet selected
    }
    
    // Calculate camera position based on updated target
    float yawRad = cameraYaw * 3.14159265f / 180.0f;
    float pitchRad = cameraPitch * 3.14159265f / 180.0f;
    
    camera.position.x = cameraTarget.x + cameraDistance * cosf(pitchRad) * cosf(yawRad);
    camera.position.y = cameraTarget.y + cameraDistance * sinf(pitchRad);
    camera.position.z = cameraTarget.z + cameraDistance * cosf(pitchRad) * sinf(yawRad);
    
    // Update actual camera target for raylib
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
        cameraYaw = -45.0f;
        cameraPitch = -30.0f;
    }
}

void PlanetMapGUI::Render() {
    if (!visible) return;
    
    // Update aspect ratio in case screen size changed
    aspectRatio = (float)GetScreenWidth() / (float)GetScreenHeight();
    
    // Check if render texture needs to be recreated (screen size changed)
    if (sceneTexture.texture.width != GetScreenWidth() || sceneTexture.texture.height != GetScreenHeight()) {
        UnloadRenderTexture(sceneTexture);
        sceneTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    }
    
    // --- PASS 1: Render 3D scene to texture with custom projection ---
    BeginTextureMode(sceneTexture);
    ClearBackground(BLACK);
    
    // Setup custom projection matrix with extended far plane
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    
    // Custom perspective projection with far plane at 10000
    float fovy = camera.fovy * 3.14159265f / 180.0f; // Convert to radians
    float near = 0.01f;
    float far = 10000.0f;               // Extended far plane
    
    // Calculate perspective matrix manually
    float f = 1.0f / tanf(fovy * 0.5f);
    float projection[16] = {
        f / aspectRatio, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (far + near) / (near - far), -1,
        0, 0, (2 * far * near) / (near - far), 0
    };
    
    rlMultMatrixf(projection);
    
    // Setup view matrix
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    
    // Calculate view matrix manually
    Vector3 forward = {
        camera.target.x - camera.position.x,
        camera.target.y - camera.position.y,
        camera.target.z - camera.position.z
    };
    
    // Vector3Normalize(forward)
    float forwardLength = sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    forward.x /= forwardLength;
    forward.y /= forwardLength;
    forward.z /= forwardLength;
    
    // Vector3 right = { forward.z, 0, -forward.x };
    Vector3 right = { forward.z, 0, -forward.x };
    
    // Vector3Normalize(right)
    float rightLength = sqrtf(right.x * right.x + right.y * right.y + right.z * right.z);
    right.x /= rightLength;
    right.y /= rightLength;
    right.z /= rightLength;
    
    // Vector3 up = Vector3CrossProduct(right, forward);
    Vector3 up = {
        right.y * forward.z - right.z * forward.y,
        right.z * forward.x - right.x * forward.z,
        right.x * forward.y - right.y * forward.x
    };
    
    float view[16] = {
        right.x, up.x, -forward.x, 0,
        right.y, up.y, -forward.y, 0,
        right.z, up.z, -forward.z, 0,
        -(right.x * camera.position.x + right.y * camera.position.y + right.z * camera.position.z),
        -(up.x * camera.position.x + up.y * camera.position.y + up.z * camera.position.z),
        forward.x * camera.position.x + forward.y * camera.position.y + forward.z * camera.position.z, 1
    };
    
    rlMultMatrixf(view);
    
    // Enable depth testing
    rlEnableDepthTest();
    
    // Render 3D elements
    DrawOrbits();
    
    for (const auto& planet : planets) {
        DrawPlanet(planet);
    }
    
    EndTextureMode();
    
    // --- PASS 2: Composite and render 2D UI ---
    // Draw the rendered 3D scene
    DrawTextureRec(sceneTexture.texture, {0, 0, (float)sceneTexture.texture.width, -(float)sceneTexture.texture.height}, {0, 0}, WHITE);
    
    // Draw 2D UI elements
    DrawUI();
    
    // Draw selection highlights and name tags in 2D mode
    // We use GetWorldToScreen with the same camera parameters used for the custom projection
    for (const auto& planet : planets) {
        // Calculate screen position using manual projection (matching the custom projection used in Pass 1)
        Vector3 relativePos = {
            planet.position.x - camera.position.x,
            planet.position.y - camera.position.y,
            planet.position.z - camera.position.z
        };
        
        // Project using the same perspective formula as our custom matrix
        float fovy = camera.fovy * 3.14159265f / 180.0f;
        float f = 1.0f / tanf(fovy * 0.5f);
        
        // Transform to camera space (simplified - assumes camera is looking at origin from diagonal)
        // For accurate results, we need the full view-projection transform
        // Use raylib's GetWorldToScreen as approximation
        Vector2 screenPos = GetWorldToScreen(planet.position, camera);
        
        // Only draw if on screen
        if (screenPos.x > 0 && screenPos.x < GetScreenWidth() &&
            screenPos.y > 0 && screenPos.y < GetScreenHeight()) {
            
            // Check if selected
            bool isSelected = (selectedPlanet >= 0 && planets[selectedPlanet].id == planet.id);
            
            // Draw selection highlight as 2D circle around screen position
            if (isSelected) {
                float screenRadius = planet.radius * f * aspectRatio * 40.0f; // Approximate screen-space radius
                if (screenRadius < 20.0f) screenRadius = 20.0f;
                if (screenRadius > 100.0f) screenRadius = 100.0f;
                DrawCircleLines((int)screenPos.x, (int)screenPos.y, (int)screenRadius, YELLOW);
            }
            
            // Draw name tags for selected or large planets
            if (isSelected || planet.radius > 2.0f) {
                Color textColor = isSelected ? YELLOW : WHITE;
                DrawText(planet.name.c_str(), (int)screenPos.x - 20, (int)screenPos.y - 10, 12, textColor);
            }
        }
    }
}

void PlanetMapGUI::DrawPlanet(const PlanetMapEntry& planet) {
    // Draw planet sphere only (selection highlight handled in Render)
    // Note: This is called within custom matrix mode, so we use rlgl directly
    DrawSphere(planet.position, planet.radius, planet.color);
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
    // Main controls panel - cleaner with more padding
    DrawRectangle(20, 20, 280, 120, {0, 0, 0, 180});
    DrawRectangleLines(20, 20, 280, 120, {150, 150, 150, 255});
    
    // Title - simplified
    DrawText("SYSTEM MAP", 35, 35, 20, WHITE);
    
    // Essential controls only - reduced text
    DrawText("M: Close | Click: Select | Arrows: Move", 35, 65, 14, {200, 200, 200, 255});
    DrawText("W/S: Zoom | Right-click: Rotate", 35, 85, 14, {200, 200, 200, 255});
    
    // Planet count - smaller
    char countText[64];
    sprintf(countText, "Bodies: %zu", planets.size());
    DrawText(countText, 35, 105, 12, {180, 180, 180, 255});
    
    // Selected planet info - cleaner design
    if (selectedPlanet >= 0 && selectedPlanet < (int)planets.size()) {
        const auto& p = planets[selectedPlanet];
        
        // Right panel with better padding
        DrawRectangle(GetScreenWidth() - 300, 20, 280, 100, {0, 0, 0, 180});
        DrawRectangleLines(GetScreenWidth() - 300, 20, 280, 100, YELLOW);
        
        // Planet name - prominent
        DrawText(p.name.c_str(), GetScreenWidth() - 290, 35, 18, WHITE);
        
        // Essential info only - cleaner layout
        char info[128];
        std::string typeStr = "Planet";
        if (p.id == "sandos") {
            typeStr = "Star";
        } else if (p.isMoon) {
            typeStr = "Moon";
        }
        
        // Type and radius on one line
        sprintf(info, "%s | %.1f Earth radii", typeStr.c_str(), p.actualRadius);
        DrawText(info, GetScreenWidth() - 290, 60, 14, {200, 200, 200, 255});
        
        // Smaller km info
        sprintf(info, "~%.0f km", p.actualRadius * 6371.0f);
        DrawText(info, GetScreenWidth() - 290, 80, 12, {160, 160, 160, 255});
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
