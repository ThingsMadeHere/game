#pragma once
#include <vector>
#include <string>

// Noise functions
float SmoothNoise3D(float x, float y, float z);
float FBM(float x, float y, float z, int octaves);

// Advanced terrain generation
float GetTerrainHeightAdvanced(float worldX, float worldZ);
float GetCaveDensity(float worldX, float worldY, float worldZ);

// Noise layer definition
struct NoiseLayerDef {
    const char* name;
    float amplitude;
    float frequency;
    int octaves;
    float persistence;
    float lacunarity;
    float offset;
};

// Planet type noise presets
struct PlanetNoisePreset {
    const char* planetType;
    std::vector<NoiseLayerDef> layers;
};

// Get noise preset for a planet type
const std::vector<NoiseLayerDef>* GetPlanetNoisePreset(const std::string& planetType);

// Initialize planet noise system
void InitPlanetNoisePresets();
