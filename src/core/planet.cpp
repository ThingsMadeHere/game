#include "planet.h"
#include "../terrain/noise.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cstring>

// Simple JSON parsing helpers (minimal implementation)
namespace {
    std::string Trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
    
    std::string ExtractStringValue(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return "";
        
        pos = json.find(":", pos);
        if (pos == std::string::npos) return "";
        
        pos = json.find("\"", pos + 1);
        if (pos == std::string::npos) return "";
        
        size_t endPos = json.find("\"", pos + 1);
        if (endPos == std::string::npos) return "";
        
        return json.substr(pos + 1, endPos - pos - 1);
    }
    
    float ExtractFloatValue(const std::string& json, const std::string& key, float defaultValue = 0.0f) {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return defaultValue;
        
        pos = json.find(":", pos);
        if (pos == std::string::npos) return defaultValue;
        
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        
        char* endPtr;
        float value = strtof(json.c_str() + pos, &endPtr);
        if (endPtr == json.c_str() + pos) return defaultValue;
        
        return value;
    }
    
    int ExtractIntValue(const std::string& json, const std::string& key, int defaultValue = 0) {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return defaultValue;
        
        pos = json.find(":", pos);
        if (pos == std::string::npos) return defaultValue;
        
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        
        char* endPtr;
        int value = (int)strtol(json.c_str() + pos, &endPtr, 10);
        if (endPtr == json.c_str() + pos) return defaultValue;
        
        return value;
    }
    
    bool ExtractBoolValue(const std::string& json, const std::string& key, bool defaultValue = false) {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return defaultValue;
        
        pos = json.find(":", pos);
        if (pos == std::string::npos) return defaultValue;
        
        if (json.find("true", pos) < json.find(",", pos) && json.find("true", pos) < json.find("}", pos)) {
            return true;
        }
        if (json.find("false", pos) < json.find(",", pos) && json.find("false", pos) < json.find("}", pos)) {
            return false;
        }
        
        return defaultValue;
    }
    
    std::vector<std::string> ExtractStringArray(const std::string& json, const std::string& key) {
        std::vector<std::string> result;
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return result;
        
        pos = json.find("[", pos);
        if (pos == std::string::npos) return result;
        
        size_t endPos = json.find("]", pos);
        if (endPos == std::string::npos) return result;
        
        std::string arrayContent = json.substr(pos + 1, endPos - pos - 1);
        
        size_t start = 0;
        while ((start = arrayContent.find("\"", start)) != std::string::npos) {
            size_t end = arrayContent.find("\"", start + 1);
            if (end == std::string::npos) break;
            result.push_back(arrayContent.substr(start + 1, end - start - 1));
            start = end + 1;
        }
        
        return result;
    }
}

// Global instance
PlanetSystem g_PlanetSystem;

// ============================================================================
// PlanetSystem Implementation
// ============================================================================

bool PlanetSystem::LoadPlanetFromJSON(const std::string& jsonPath) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        printf("ERROR: Could not open planet file: %s\n", jsonPath.c_str());
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    file.close();
    
    PlanetDefinition planet;
    
    // Basic info
    planet.id = ExtractStringValue(json, "id");
    planet.name = ExtractStringValue(json, "name");
    planet.description = ExtractStringValue(json, "description");
    planet.planetType = ExtractStringValue(json, "planetType");
    
    if (planet.id.empty()) {
        printf("ERROR: Planet definition missing 'id' field in %s\n", jsonPath.c_str());
        return false;
    }
    
    // Orbital parameters
    planet.orbit.semiMajorAxis = ExtractFloatValue(json, "semiMajorAxis", 1000.0f);
    planet.orbit.eccentricity = ExtractFloatValue(json, "eccentricity", 0.0f);
    planet.orbit.inclination = ExtractFloatValue(json, "inclination", 0.0f);
    planet.orbit.longitudeOfAscendingNode = ExtractFloatValue(json, "longitudeOfAscendingNode", 0.0f);
    planet.orbit.argumentOfPeriapsis = ExtractFloatValue(json, "argumentOfPeriapsis", 0.0f);
    planet.orbit.meanAnomalyAtEpoch = ExtractFloatValue(json, "meanAnomalyAtEpoch", 0.0f);
    planet.orbit.orbitalPeriod = ExtractFloatValue(json, "orbitalPeriod", 365.0f);
    planet.orbit.parentObjectId = ExtractStringValue(json, "parentObjectId");
    
    // Physical parameters
    planet.physical.radius = ExtractFloatValue(json, "radius", 50.0f);
    planet.physical.mass = ExtractFloatValue(json, "mass", 1.0f);
    planet.physical.density = ExtractFloatValue(json, "density", 5.5f);
    planet.physical.surfaceGravity = ExtractFloatValue(json, "surfaceGravity", 9.81f);
    planet.physical.rotationalPeriod = ExtractFloatValue(json, "rotationalPeriod", 24.0f);
    planet.physical.axialTilt = ExtractFloatValue(json, "axialTilt", 23.5f);
    
    // Atmospheric parameters
    planet.atmosphere.hasAtmosphere = ExtractBoolValue(json, "hasAtmosphere", true);
    planet.atmosphere.surfacePressure = ExtractFloatValue(json, "surfacePressure", 1.0f);
    planet.atmosphere.scaleHeight = ExtractFloatValue(json, "scaleHeight", 8.5f);
    planet.atmosphere.atmosphericHeight = ExtractFloatValue(json, "atmosphericHeight", 100.0f);
    planet.atmosphere.skyColorR = ExtractFloatValue(json, "skyColorR", 0.4f);
    planet.atmosphere.skyColorG = ExtractFloatValue(json, "skyColorG", 0.6f);
    planet.atmosphere.skyColorB = ExtractFloatValue(json, "skyColorB", 1.0f);
    planet.atmosphere.sunsetColorR = ExtractFloatValue(json, "sunsetColorR", 1.0f);
    planet.atmosphere.sunsetColorG = ExtractFloatValue(json, "sunsetColorG", 0.5f);
    planet.atmosphere.sunsetColorB = ExtractFloatValue(json, "sunsetColorB", 0.3f);
    planet.atmosphere.hazeDensity = ExtractFloatValue(json, "hazeDensity", 0.3f);
    planet.atmosphere.cloudDensity = ExtractFloatValue(json, "cloudDensity", 0.5f);
    planet.atmosphere.averageTemperature = ExtractFloatValue(json, "averageTemperature", 15.0f);
    planet.atmosphere.hasWeather = ExtractBoolValue(json, "hasWeather", true);
    
    // Terrain parameters
    planet.terrain.baseHeight = ExtractFloatValue(json, "baseHeight", 0.0f);
    planet.terrain.heightScale = ExtractFloatValue(json, "heightScale", 1.0f);
    planet.terrain.waterLevel = ExtractFloatValue(json, "waterLevel", 0.3f);
    planet.terrain.cliffThreshold = ExtractFloatValue(json, "cliffThreshold", 0.7f);
    
    // Visual parameters
    planet.visual.baseColor.r = (unsigned char)ExtractIntValue(json, "baseColorR", 100);
    planet.visual.baseColor.g = (unsigned char)ExtractIntValue(json, "baseColorG", 140);
    planet.visual.baseColor.b = (unsigned char)ExtractIntValue(json, "baseColorB", 100);
    planet.visual.baseColor.a = 255;
    
    planet.visual.oceanColor.r = (unsigned char)ExtractIntValue(json, "oceanColorR", 30);
    planet.visual.oceanColor.g = (unsigned char)ExtractIntValue(json, "oceanColorG", 60);
    planet.visual.oceanColor.b = (unsigned char)ExtractIntValue(json, "oceanColorB", 120);
    planet.visual.oceanColor.a = 255;
    
    planet.visual.specularIntensity = ExtractFloatValue(json, "specularIntensity", 0.3f);
    planet.visual.hasRings = ExtractBoolValue(json, "hasRings", false);
    planet.visual.ringInnerRadius = ExtractFloatValue(json, "ringInnerRadius", 0.0f);
    planet.visual.ringOuterRadius = ExtractFloatValue(json, "ringOuterRadius", 0.0f);
    
    // Special properties
    planet.isHabitable = ExtractBoolValue(json, "isHabitable", false);
    planet.hasMagneticField = ExtractBoolValue(json, "hasMagneticField", false);
    planet.tidallyLocked = ExtractBoolValue(json, "tidallyLocked", false);
    
    // Moon IDs
    planet.moonIds = ExtractStringArray(json, "moons");
    
    // Parse noise layers
    size_t noiseLayersPos = json.find("\"noiseLayers\"");
    if (noiseLayersPos != std::string::npos) {
        size_t arrayStart = json.find("[", noiseLayersPos);
        if (arrayStart != std::string::npos) {
            int bracketCount = 1;
            size_t arrayEnd = arrayStart + 1;
            while (arrayEnd < json.size() && bracketCount > 0) {
                if (json[arrayEnd] == '[') bracketCount++;
                else if (json[arrayEnd] == ']') bracketCount--;
                arrayEnd++;
            }
            
            std::string layersJson = json.substr(arrayStart, arrayEnd - arrayStart);
            
            // Parse each layer (simplified - looks for objects within the array)
            size_t objStart = 0;
            while ((objStart = layersJson.find("{", objStart)) != std::string::npos) {
                size_t objEnd = layersJson.find("}", objStart);
                if (objEnd == std::string::npos) break;
                
                std::string layerJson = layersJson.substr(objStart, objEnd - objStart + 1);
                
                NoiseLayer layer;
                layer.name = ExtractStringValue(layerJson, "name");
                layer.amplitude = ExtractFloatValue(layerJson, "amplitude", 1.0f);
                layer.frequency = ExtractFloatValue(layerJson, "frequency", 0.02f);
                layer.octaves = ExtractIntValue(layerJson, "octaves", 6);
                layer.blendMode = ExtractStringValue(layerJson, "blendMode");
                layer.blendFactor = ExtractFloatValue(layerJson, "blendFactor", 1.0f);
                
                planet.terrain.noiseLayers.push_back(layer);
                objStart = objEnd + 1;
            }
        }
    }
    
    // Parse ecosystem objects
    size_t ecoPos = json.find("\"ecosystemObjects\"");
    if (ecoPos != std::string::npos) {
        size_t arrayStart = json.find("[", ecoPos);
        if (arrayStart != std::string::npos) {
            int bracketCount = 1;
            size_t arrayEnd = arrayStart + 1;
            while (arrayEnd < json.size() && bracketCount > 0) {
                if (json[arrayEnd] == '[') bracketCount++;
                else if (json[arrayEnd] == ']') bracketCount--;
                arrayEnd++;
            }
            
            std::string objectsJson = json.substr(arrayStart, arrayEnd - arrayStart);
            
            size_t objStart = 0;
            while ((objStart = objectsJson.find("{", objStart)) != std::string::npos) {
                size_t objEnd = objectsJson.find("}", objStart);
                if (objEnd == std::string::npos) break;
                
                std::string objJson = objectsJson.substr(objStart, objEnd - objStart + 1);
                
                EcosystemObject obj;
                obj.id = ExtractStringValue(objJson, "id");
                obj.type = ExtractStringValue(objJson, "type");
                obj.name = ExtractStringValue(objJson, "name");
                obj.modelPath = ExtractStringValue(objJson, "modelPath");
                obj.texturePath = ExtractStringValue(objJson, "texturePath");
                obj.minAltitude = ExtractFloatValue(objJson, "minAltitude", -1000.0f);
                obj.maxAltitude = ExtractFloatValue(objJson, "maxAltitude", 1000.0f);
                obj.baseDensity = ExtractFloatValue(objJson, "baseDensity", 1.0f);
                obj.minScale = ExtractFloatValue(objJson, "minScale", 0.8f);
                obj.maxScale = ExtractFloatValue(objJson, "maxScale", 1.2f);
                obj.hasCollision = ExtractBoolValue(objJson, "hasCollision", true);
                obj.canBeHarvested = ExtractBoolValue(objJson, "canBeHarvested", false);
                obj.allowedBiomes = ExtractStringArray(objJson, "allowedBiomes");
                obj.requiredBiomes = ExtractStringArray(objJson, "requiredBiomes");
                
                if (!obj.id.empty()) {
                    planet.ecosystem.objects.push_back(obj);
                }
                
                objStart = objEnd + 1;
            }
        }
    }
    
    // Add to collection
    planets[planet.id] = planet;
    printf("Loaded planet: %s (%s)\n", planet.name.c_str(), planet.id.c_str());
    
    return true;
}

bool PlanetSystem::LoadAllPlanetsFromDirectory(const std::string& directory) {
    std::string dir = directory.empty() ? dataDirectory : directory;
    printf("Loading planets from directory: %s\n", dir.c_str());
    
    // For now, this is a placeholder - would need filesystem iteration
    // In a real implementation, you'd use std::filesystem or platform-specific code
    
    // Try loading all planet files including new Sandos system
    std::vector<std::string> defaultFiles = {
        dir + "earth.json",
        dir + "mars.json",
        dir + "moon.json",
        dir + "sandos.json",
        dir + "proxmai.json",
        dir + "sulfos.json",
        dir + "oceanus.json",
        dir + "etaui.json",
        dir + "etauos.json",
        dir + "massivo.json",
        dir + "vulcan.json",
        dir + "glacieo.json",
        dir + "infinatos.json"
    };
    
    int loaded = 0;
    for (const auto& file : defaultFiles) {
        if (LoadPlanetFromJSON(file)) {
            loaded++;
        }
    }
    
    printf("Loaded %d planets from %s\n", loaded, dir.c_str());
    return loaded > 0;
}

bool PlanetSystem::SavePlanetToJSON(const PlanetDefinition& planet, const std::string& jsonPath) {
    std::ofstream file(jsonPath);
    if (!file.is_open()) {
        printf("ERROR: Could not open file for writing: %s\n", jsonPath.c_str());
        return false;
    }
    
    file << "{\n";
    file << "    \"id\": \"" << planet.id << "\",\n";
    file << "    \"name\": \"" << planet.name << "\",\n";
    file << "    \"description\": \"" << planet.description << "\",\n";
    file << "    \"planetType\": \"" << planet.planetType << "\",\n\n";
    
    // Orbital parameters
    file << "    \"semiMajorAxis\": " << planet.orbit.semiMajorAxis << ",\n";
    file << "    \"eccentricity\": " << planet.orbit.eccentricity << ",\n";
    file << "    \"inclination\": " << planet.orbit.inclination << ",\n";
    file << "    \"orbitalPeriod\": " << planet.orbit.orbitalPeriod << ",\n";
    if (!planet.orbit.parentObjectId.empty()) {
        file << "    \"parentObjectId\": \"" << planet.orbit.parentObjectId << "\",\n";
    }
    file << "\n";
    
    // Physical parameters
    file << "    \"radius\": " << planet.physical.radius << ",\n";
    file << "    \"mass\": " << planet.physical.mass << ",\n";
    file << "    \"surfaceGravity\": " << planet.physical.surfaceGravity << ",\n";
    file << "    \"rotationalPeriod\": " << planet.physical.rotationalPeriod << ",\n";
    file << "    \"axialTilt\": " << planet.physical.axialTilt << ",\n\n";
    
    // Atmospheric parameters
    file << "    \"hasAtmosphere\": " << (planet.atmosphere.hasAtmosphere ? "true" : "false") << ",\n";
    file << "    \"surfacePressure\": " << planet.atmosphere.surfacePressure << ",\n";
    file << "    \"skyColorR\": " << planet.atmosphere.skyColorR << ",\n";
    file << "    \"skyColorG\": " << planet.atmosphere.skyColorG << ",\n";
    file << "    \"skyColorB\": " << planet.atmosphere.skyColorB << ",\n";
    file << "    \"averageTemperature\": " << planet.atmosphere.averageTemperature << ",\n\n";
    
    // Terrain parameters
    file << "    \"baseHeight\": " << planet.terrain.baseHeight << ",\n";
    file << "    \"heightScale\": " << planet.terrain.heightScale << ",\n";
    file << "    \"waterLevel\": " << planet.terrain.waterLevel << ",\n";
    
    // Noise layers
    file << "    \"noiseLayers\": [\n";
    for (size_t i = 0; i < planet.terrain.noiseLayers.size(); i++) {
        const auto& layer = planet.terrain.noiseLayers[i];
        file << "        {\n";
        file << "            \"name\": \"" << layer.name << "\",\n";
        file << "            \"amplitude\": " << layer.amplitude << ",\n";
        file << "            \"frequency\": " << layer.frequency << ",\n";
        file << "            \"octaves\": " << layer.octaves << ",\n";
        file << "            \"blendMode\": \"" << layer.blendMode << "\"\n";
        file << "        }";
        if (i < planet.terrain.noiseLayers.size() - 1) file << ",";
        file << "\n";
    }
    file << "    ],\n\n";
    
    // Visual parameters
    file << "    \"baseColorR\": " << (int)planet.visual.baseColor.r << ",\n";
    file << "    \"baseColorG\": " << (int)planet.visual.baseColor.g << ",\n";
    file << "    \"baseColorB\": " << (int)planet.visual.baseColor.b << ",\n";
    file << "    \"specularIntensity\": " << planet.visual.specularIntensity << ",\n";
    file << "    \"hasRings\": " << (planet.visual.hasRings ? "true" : "false") << ",\n\n";
    
    // Special properties
    file << "    \"isHabitable\": " << (planet.isHabitable ? "true" : "false") << ",\n";
    file << "    \"hasMagneticField\": " << (planet.hasMagneticField ? "true" : "false") << ",\n";
    file << "    \"tidallyLocked\": " << (planet.tidallyLocked ? "true" : "false") << ",\n\n";
    
    // Moons
    file << "    \"moons\": [";
    for (size_t i = 0; i < planet.moonIds.size(); i++) {
        file << "\"" << planet.moonIds[i] << "\"";
        if (i < planet.moonIds.size() - 1) file << ", ";
    }
    file << "],\n\n";
    
    // Ecosystem objects
    file << "    \"ecosystemObjects\": [\n";
    for (size_t i = 0; i < planet.ecosystem.objects.size(); i++) {
        const auto& obj = planet.ecosystem.objects[i];
        file << "        {\n";
        file << "            \"id\": \"" << obj.id << "\",\n";
        file << "            \"type\": \"" << obj.type << "\",\n";
        file << "            \"name\": \"" << obj.name << "\",\n";
        file << "            \"modelPath\": \"" << obj.modelPath << "\",\n";
        file << "            \"texturePath\": \"" << obj.texturePath << "\",\n";
        file << "            \"minAltitude\": " << obj.minAltitude << ",\n";
        file << "            \"maxAltitude\": " << obj.maxAltitude << ",\n";
        file << "            \"baseDensity\": " << obj.baseDensity << ",\n";
        file << "            \"hasCollision\": " << (obj.hasCollision ? "true" : "false") << ",\n";
        file << "            \"canBeHarvested\": " << (obj.canBeHarvested ? "true" : "false") << "\n";
        file << "        }";
        if (i < planet.ecosystem.objects.size() - 1) file << ",";
        file << "\n";
    }
    file << "    ]\n";
    
    file << "}\n";
    file.close();
    
    printf("Saved planet: %s to %s\n", planet.name.c_str(), jsonPath.c_str());
    return true;
}

PlanetDefinition* PlanetSystem::GetPlanet(const std::string& id) {
    auto it = planets.find(id);
    return (it != planets.end()) ? &it->second : nullptr;
}

const PlanetDefinition* PlanetSystem::GetPlanet(const std::string& id) const {
    auto it = planets.find(id);
    return (it != planets.end()) ? &it->second : nullptr;
}

PlanetDefinition* PlanetSystem::GetActivePlanet() {
    if (activePlanetId.empty()) return nullptr;
    return GetPlanet(activePlanetId);
}

void PlanetSystem::SetActivePlanet(const std::string& id) {
    if (HasPlanet(id)) {
        activePlanetId = id;
        printf("Set active planet to: %s\n", id.c_str());
    } else {
        printf("WARNING: Cannot set active planet - planet '%s' not found\n", id.c_str());
    }
}

bool PlanetSystem::AddPlanet(const PlanetDefinition& planet) {
    if (planet.id.empty()) {
        printf("ERROR: Cannot add planet with empty ID\n");
        return false;
    }
    
    if (HasPlanet(planet.id)) {
        printf("WARNING: Planet '%s' already exists, updating...\n", planet.id.c_str());
    }
    
    planets[planet.id] = planet;
    return true;
}

bool PlanetSystem::RemovePlanet(const std::string& id) {
    auto it = planets.find(id);
    if (it != planets.end()) {
        planets.erase(it);
        if (activePlanetId == id) {
            activePlanetId = "";
        }
        return true;
    }
    return false;
}

bool PlanetSystem::HasPlanet(const std::string& id) const {
    return planets.find(id) != planets.end();
}

std::vector<std::string> PlanetSystem::GetAllPlanetIds() const {
    std::vector<std::string> ids;
    ids.reserve(planets.size());
    for (const auto& [id, planet] : planets) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<const PlanetDefinition*> PlanetSystem::GetPlanetsInOrbit(const std::string& parentId) const {
    std::vector<const PlanetDefinition*> result;
    for (const auto& [id, planet] : planets) {
        if (planet.orbit.parentObjectId == parentId) {
            result.push_back(&planet);
        }
    }
    return result;
}

std::vector<const PlanetDefinition*> PlanetSystem::GetHabitablePlanets() const {
    std::vector<const PlanetDefinition*> result;
    for (const auto& [id, planet] : planets) {
        if (planet.isHabitable) {
            result.push_back(&planet);
        }
    }
    return result;
}

void PlanetSystem::UpdateOrbits(float deltaTime) {
    for (auto& [id, planet] : planets) {
        if (planet.orbit.orbitalPeriod > 0) {
            // Update orbital angle based on period
            float degreesPerDay = 360.0f / planet.orbit.orbitalPeriod;
            planet.orbit.currentOrbitalAngle += degreesPerDay * deltaTime;
            if (planet.orbit.currentOrbitalAngle >= 360.0f) {
                planet.orbit.currentOrbitalAngle -= 360.0f;
            }
        }
    }
}

void PlanetSystem::UpdateRotation(float deltaTime) {
    for (auto& [id, planet] : planets) {
        if (planet.physical.rotationalPeriod > 0) {
            // Update rotation angle (convert hours to days for calculation)
            float rotationsPerDay = 24.0f / planet.physical.rotationalPeriod;
            planet.physical.currentRotationAngle += rotationsPerDay * 360.0f * deltaTime;
            if (planet.physical.currentRotationAngle >= 360.0f) {
                planet.physical.currentRotationAngle -= 360.0f;
            }
        }
    }
}

void PlanetSystem::ApplyTerrainNoise(float x, float y, float z, float& outDensity,
                                    unsigned char& outMaterial, const std::string& planetId) {
    const PlanetDefinition* planet = GetPlanet(planetId);
    if (!planet) {
        outDensity = 0.0f;
        outMaterial = 0;
        return;
    }
    
    // Get noise preset for this planet type
    const std::vector<NoiseLayerDef>* preset = GetPlanetNoisePreset(planet->planetType);
    if (!preset || preset->empty()) {
        // Fallback to simple terrain
        float height = 8.0f;
        if (y < height) {
            outDensity = 1.0f;
            outMaterial = (y < height - 2.0f) ? 3 : 1;
        } else {
            outDensity = -1.0f;
            outMaterial = 0;
        }
        return;
    }
    
    // Build a map of blend configurations from JSON by layer name
    std::unordered_map<std::string, std::pair<std::string, float>> blendConfigs;
    for (const auto& layer : planet->terrain.noiseLayers) {
        blendConfigs[layer.name] = {layer.blendMode, layer.blendFactor};
    }
    
    // Evaluate noise layers using preset parameters with JSON blend settings
    float noiseValue = 0.0f;
    float totalWeight = 0.0f;
    
    for (const auto& def : *preset) {
        // Use 2D noise (X, Z) for terrain surface height
        float layerNoise = FBM(x * def.frequency, z * def.frequency, 0.0f, def.octaves);
        layerNoise = layerNoise * def.amplitude + def.offset;
        
        // Get blend configuration from JSON (defaults to "add" with factor 1.0)
        std::string blendMode = "add";
        float blendFactor = 1.0f;
        auto it = blendConfigs.find(def.name);
        if (it != blendConfigs.end()) {
            blendMode = it->second.first;
            blendFactor = it->second.second;
            if (blendFactor == 0.0f) blendFactor = 1.0f; // Default if not set
        }
        
        // Apply blend mode
        if (blendMode == "add") {
            noiseValue += layerNoise * blendFactor;
        } else if (blendMode == "multiply") {
            noiseValue *= layerNoise;
        } else if (blendMode == "max") {
            noiseValue = fmaxf(noiseValue, layerNoise);
        } else if (blendMode == "min") {
            noiseValue = fminf(noiseValue, layerNoise);
        } else if (blendMode == "subtract") {
            noiseValue -= layerNoise * blendFactor;
        } else if (blendMode == "lerp") {
            noiseValue = noiseValue * (1.0f - blendFactor) + layerNoise * blendFactor;
        } else {
            // Default to add
            noiseValue += layerNoise * blendFactor;
        }
        
        totalWeight += def.amplitude;
    }
    
    // Normalize
    if (totalWeight > 0) {
        noiseValue /= totalWeight;
    }
    
    // Calculate surface height using planet's baseHeight and heightScale from JSON
    float surfaceHeight = planet->terrain.baseHeight + noiseValue * planet->terrain.heightScale;
    
    // Determine density and material
    if (y < surfaceHeight) {
        outDensity = 1.0f;
        
        float depth = surfaceHeight - y;
        if (depth > 5.0f) {
            outMaterial = 3; // Stone
        } else if (depth > 1.0f) {
            outMaterial = 2; // Dirt
        } else {
            outMaterial = 1; // Surface
        }
    } else {
        outDensity = -1.0f;
        outMaterial = 0; // Air
    }
}

std::vector<EcosystemObject> PlanetSystem::GetSpawnableObjects(
    const std::string& planetId,
    float altitude,
    float temperature,
    float moisture,
    const std::string& biome) const {
    
    std::vector<EcosystemObject> result;
    const PlanetDefinition* planet = GetPlanet(planetId);
    
    if (!planet) {
        return result;
    }
    
    for (const auto& obj : planet->ecosystem.objects) {
        // Check altitude
        if (altitude < obj.minAltitude || altitude > obj.maxAltitude) {
            continue;
        }
        
        // Check temperature
        if (temperature < obj.minTemperature || temperature > obj.maxTemperature) {
            continue;
        }
        
        // Check moisture
        if (moisture < obj.minMoisture || moisture > obj.maxMoisture) {
            continue;
        }
        
        // Check biomes
        if (!obj.allowedBiomes.empty()) {
            bool allowed = false;
            for (const auto& allowedBiome : obj.allowedBiomes) {
                if (allowedBiome == biome) {
                    allowed = true;
                    break;
                }
            }
            if (!allowed) continue;
        }
        
        if (!obj.requiredBiomes.empty()) {
            bool hasRequired = false;
            for (const auto& requiredBiome : obj.requiredBiomes) {
                if (requiredBiome == biome) {
                    hasRequired = true;
                    break;
                }
            }
            if (!hasRequired) continue;
        }
        
        result.push_back(obj);
    }
    
    return result;
}

// ============================================================================
// PlanetUtils Implementation
// ============================================================================

Vector3 PlanetUtils::CalculateOrbitalPosition(const OrbitalParameters& orbit, float time) {
    // Simplified orbital calculation (circular orbit approximation)
    float angle = orbit.currentOrbitalAngle + (360.0f / orbit.orbitalPeriod) * time;
    float rad = angle * (3.14159265359f / 180.0f);
    
    // Apply inclination
    float incRad = orbit.inclination * (3.14159265359f / 180.0f);
    
    Vector3 position;
    position.x = orbit.semiMajorAxis * cosf(rad);
    position.y = orbit.semiMajorAxis * sinf(rad) * sinf(incRad);
    position.z = orbit.semiMajorAxis * sinf(rad) * cosf(incRad);
    
    return position;
}

float PlanetUtils::CalculateSurfaceGravity(float mass, float radius) {
    // G * M / R^2 (simplified, using Earth as reference)
    const float G = 6.674e-11f;
    const float earthMass = 5.972e24f;
    const float earthRadius = 6.371e6f;
    
    float actualMass = mass * earthMass;
    float actualRadius = radius * earthRadius;
    
    return G * actualMass / (actualRadius * actualRadius);
}

float PlanetUtils::CalculateEquilibriumTemperature(float distanceFromStar, float albedo, float stellarLuminosity) {
    // Simplified equilibrium temperature calculation
    const float solarConstant = 1361.0f; // W/m² at Earth
    const float stefanBoltzmann = 5.67e-8f;
    
    float incidentFlux = solarConstant * stellarLuminosity / (distanceFromStar * distanceFromStar);
    float absorbedFlux = incidentFlux * (1.0f - albedo) / 4.0f;
    
    float temperature = powf(absorbedFlux / stefanBoltzmann, 0.25f);
    return temperature - 273.15f; // Convert to Celsius
}

float PlanetUtils::EvaluateNoiseLayers(const std::vector<NoiseLayer>& layers,
                                      float x, float y, float z) {
    float noiseValue = 0.0f;
    float totalAmplitude = 0.0f;
    
    for (const auto& layer : layers) {
        float layerNoise = FBM(x * layer.frequency, y * layer.frequency, z * layer.frequency, layer.octaves);
        layerNoise = layerNoise * layer.amplitude + layer.offset;
        
        if (layer.blendMode == "add") {
            noiseValue += layerNoise * layer.blendFactor;
        } else if (layer.blendMode == "multiply") {
            noiseValue *= layerNoise;
        } else if (layer.blendMode == "max") {
            noiseValue = fmaxf(noiseValue, layerNoise);
        } else if (layer.blendMode == "min") {
            noiseValue = fminf(noiseValue, layerNoise);
        } else if (layer.blendMode == "lerp") {
            noiseValue = noiseValue * (1.0f - layer.blendFactor) + layerNoise * layer.blendFactor;
        }
        
        totalAmplitude += layer.amplitude;
    }
    
    if (totalAmplitude > 0) {
        noiseValue /= totalAmplitude;
    }
    
    return noiseValue;
}

bool PlanetUtils::CanSpawnObject(const EcosystemObject& obj, float altitude, float temperature,
                                float moisture, const std::string& biome) {
    if (altitude < obj.minAltitude || altitude > obj.maxAltitude) return false;
    if (temperature < obj.minTemperature || temperature > obj.maxTemperature) return false;
    if (moisture < obj.minMoisture || moisture > obj.maxMoisture) return false;
    
    if (!obj.allowedBiomes.empty()) {
        bool allowed = false;
        for (const auto& allowedBiome : obj.allowedBiomes) {
            if (allowedBiome == biome) {
                allowed = true;
                break;
            }
        }
        if (!allowed) return false;
    }
    
    if (!obj.requiredBiomes.empty()) {
        bool hasRequired = false;
        for (const auto& requiredBiome : obj.requiredBiomes) {
            if (requiredBiome == biome) {
                hasRequired = true;
                break;
            }
        }
        if (!hasRequired) return false;
    }
    
    return true;
}
