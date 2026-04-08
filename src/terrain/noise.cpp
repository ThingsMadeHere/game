#include "noise.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <unordered_map>

// ============================================================================
// Noise Presets for Planet Types
// ============================================================================

static std::unordered_map<std::string, std::vector<NoiseLayerDef>> g_noisePresets;

void InitPlanetNoisePresets() {
    // Terrestrial - Earth-like (continents, mountains, hills, detail)
    g_noisePresets["terrestrial"] = {
        {"continents", 1.0f, 0.008f, 4, 0.5f, 2.0f, 0.0f},
        {"mountains", 0.6f, 0.025f, 5, 0.5f, 2.0f, 0.0f},
        {"hills", 0.3f, 0.06f, 4, 0.5f, 2.0f, 0.0f},
        {"detail", 0.15f, 0.15f, 3, 0.5f, 2.0f, 0.0f}
    };
    
    // Super-Earth / Heavy gravity - smoother, larger features
    g_noisePresets["superearth"] = {
        {"tectonic", 1.2f, 0.005f, 5, 0.5f, 2.0f, 0.0f},
        {"mountains", 0.5f, 0.02f, 6, 0.5f, 2.0f, 0.0f},
        {"hills", 0.25f, 0.05f, 4, 0.5f, 2.0f, 0.0f},
        {"detail", 0.1f, 0.12f, 3, 0.5f, 2.0f, 0.0f}
    };
    
    // Volcanic - rough, chaotic, lava flows
    g_noisePresets["volcanic"] = {
        {"calderas", 1.0f, 0.015f, 4, 0.6f, 2.0f, 0.0f},
        {"lava_flows", 0.7f, 0.03f, 5, 0.6f, 2.0f, 0.0f},
        {"rough_terrain", 0.5f, 0.08f, 4, 0.6f, 2.0f, 0.0f},
        {"detail", 0.3f, 0.2f, 3, 0.6f, 2.0f, 0.0f}
    };
    
    // Ocean world - islands, seafloor
    g_noisePresets["ocean"] = {
        {"islands", 1.0f, 0.012f, 3, 0.5f, 2.0f, 0.0f},
        {"seafloor", 0.4f, 0.035f, 5, 0.5f, 2.0f, 0.0f},
        {"detail", 0.2f, 0.1f, 4, 0.5f, 2.0f, 0.0f}
    };
    
    // Ice world - smooth with pressure ridges, crevasses
    g_noisePresets["ice"] = {
        {"ice_sheets", 0.8f, 0.008f, 4, 0.4f, 2.0f, 0.0f},
        {"crevasses", 0.4f, 0.025f, 5, 0.4f, 2.0f, 0.0f},
        {"pressure_ridges", 0.3f, 0.06f, 4, 0.4f, 2.0f, 0.0f},
        {"detail", 0.15f, 0.15f, 3, 0.4f, 2.0f, 0.0f}
    };
    
    // Gas giant - banding, storms, swirls (for visualization)
    g_noisePresets["gas_giant"] = {
        {"banding", 0.5f, 0.005f, 3, 0.5f, 2.0f, 0.0f},
        {"storms", 0.4f, 0.02f, 5, 0.5f, 2.0f, 0.0f},
        {"swirls", 0.3f, 0.04f, 4, 0.5f, 2.0f, 0.0f}
    };
    
    // Default - simple terrain
    g_noisePresets["default"] = {
        {"base", 1.0f, 0.02f, 4, 0.5f, 2.0f, 0.0f},
        {"detail", 0.3f, 0.1f, 3, 0.5f, 2.0f, 0.0f}
    };
}

const std::vector<NoiseLayerDef>* GetPlanetNoisePreset(const std::string& planetType) {
    auto it = g_noisePresets.find(planetType);
    if (it != g_noisePresets.end()) {
        return &it->second;
    }
    // Fallback to default
    it = g_noisePresets.find("default");
    if (it != g_noisePresets.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// Noise Functions
// ============================================================================

// Improved Perlin-like noise implementation
float Hash(float x, float y, float z) {
    // Better hash function for smoother noise
    int xi = ((int)floorf(x)) & 255;
    int yi = ((int)floorf(y)) & 255;
    int zi = ((int)floorf(z)) & 255;
    
    // Handle negative coordinates properly
    if (xi < 0) xi += 256;
    if (yi < 0) yi += 256;
    if (zi < 0) zi += 256;
    
    // Simple permutation table simulation
    int hash = (xi * 374761393 + yi * 668265263 + zi * 217460973) % 256;
    return (float)hash / 256.0f;
}

float SmoothNoise3D(float x, float y, float z) {
    // Get integer coordinates
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    int iz = (int)floorf(z);
    
    // Get fractional coordinates
    float fx = x - ix;
    float fy = y - iy;
    float fz = z - iz;
    
    // Get hash values for corners
    float h000 = Hash((float)ix, (float)iy, (float)iz);
    float h001 = Hash((float)ix, (float)iy, (float)(iz + 1));
    float h010 = Hash((float)ix, (float)(iy + 1), (float)iz);
    float h011 = Hash((float)ix, (float)(iy + 1), (float)(iz + 1));
    float h100 = Hash((float)(ix + 1), (float)iy, (float)iz);
    float h101 = Hash((float)(ix + 1), (float)iy, (float)(iz + 1));
    float h110 = Hash((float)(ix + 1), (float)(iy + 1), (float)iz);
    float h111 = Hash((float)(ix + 1), (float)(iy + 1), (float)(iz + 1));
    
    // Smoothstep interpolation
    float u = fx * fx * (3.0f - 2.0f * fx);
    float v = fy * fy * (3.0f - 2.0f * fy);
    float w = fz * fz * (3.0f - 2.0f * fz);
    
    // Trilinear interpolation
    float x00 = h000 * (1.0f - u) + h100 * u;
    float x01 = h001 * (1.0f - u) + h101 * u;
    float x10 = h010 * (1.0f - u) + h110 * u;
    float x11 = h011 * (1.0f - u) + h111 * u;
    
    float y0 = x00 * (1.0f - v) + x10 * v;
    float y1 = x01 * (1.0f - v) + x11 * v;
    
    return y0 * (1.0f - w) + y1 * w;
}

float FBM(float x, float y, float z, int octaves) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 0.05f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        value += SmoothNoise3D(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return value / maxValue;
}
