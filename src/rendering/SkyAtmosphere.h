#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Configuration for atmospheric scattering
struct AtmosphereConfig {
    // Planet properties
    float planetRadius = 6371000.0f;      // Earth radius in meters
    float atmosphereDepth = 80000.0f;     // Thickness of atmosphere in meters
    
    // Scattering coefficients (Rayleigh)
    // Corresponds to wavelengths: ~680nm (R), ~550nm (G), ~440nm (B)
    glm::vec3 rayleighScattering = glm::vec3(5.80e-6f, 13.56e-6f, 33.10e-6f);
    
    // Mie scattering coefficient (approximation for haze/dust)
    float mieScattering = 21e-6f;
    
    // Sun intensity
    float sunIntensity = 20.0f;
    
    // Rayleigh/Mie anisotropy (g factor for Mie phase function)
    float mieAnisotropy = 0.76f;
};

class SkyAtmosphere {
public:
    SkyAtmosphere();
    ~SkyAtmosphere();

    void Initialize();
    void Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& sunDirection);
    void Cleanup();

    void SetConfig(const AtmosphereConfig& config);
    const AtmosphereConfig& GetConfig() const { return m_config; }

private:
    void CreateGeometry();
    void LoadShaders();
    void CalculateScatteringCoefficients();

    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_shaderProgram;

    AtmosphereConfig m_config;
    
    // Uniform locations
    int m_locViewProj;
    int m_locSunDirection;
    int m_locRayleighColor;
    int m_locMieColor;
    int m_locSunIntensity;
    int m_locPlanetRadius;
    int m_locAtmosphereDepth;
    int m_locMieAnisotropy;
};
