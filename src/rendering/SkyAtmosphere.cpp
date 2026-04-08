#include "SkyAtmosphere.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>

// Vertex shader for sky dome
static const char* skyVertexShaderSource = R"(
#version 450 core

layout(location = 0) in vec3 aPos;

uniform mat4 uViewProj;
uniform vec3 uCameraPos;

out vec3 vWorldPos;

void main() {
    // Position vertex at infinity (sky dome)
    vec4 worldPos = vec4(uCameraPos + normalize(aPos) * 10000.0, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = uViewProj * worldPos;
    
    // Remove depth testing for sky
    gl_Position.z = gl_Position.w;
}
)";

// Fragment shader implementing atmospheric scattering
static const char* skyFragmentShaderSource = R"(
#version 450 core

in vec3 vWorldPos;

uniform vec3 uSunDirection;
uniform vec3 uRayleighColor;
uniform vec3 uMieColor;
uniform float uSunIntensity;
uniform float uPlanetRadius;
uniform float uAtmosphereDepth;
uniform float uMieAnisotropy;

out vec4 FragColor;

// Constants
const int SAMPLE_COUNT = 16;
const float PI = 3.14159265359;

// Calculate Rayleigh phase function
float RayleighPhase(float cosTheta) {
    return 0.75 * (1.0 + cosTheta * cosTheta);
}

// Calculate Mie phase function (Henyey-Greenstein approximation)
float MiePhase(float cosTheta, float g) {
    float g2 = g * g;
    float numerator = (1.0 - g2);
    float denominator = pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return (3.0 / (8.0 * PI)) * (numerator / denominator);
}

// Calculate optical depth through atmosphere
vec3 CalculateScattering(vec3 rayDirection, vec3 sunDirection) {
    float planetRadius = uPlanetRadius;
    float atmosphereRadius = planetRadius + uAtmosphereDepth;
    
    vec3 cameraPos = vWorldPos;
    float cameraHeight = length(cameraPos) - planetRadius;
    
    // Normalize directions
    vec3 rayDir = normalize(rayDirection);
    vec3 sunDir = normalize(sunDirection);
    
    // Calculate intersection with atmosphere
    float b = 2.0 * dot(cameraPos, rayDir);
    float c = dot(cameraPos, cameraPos) - atmosphereRadius * atmosphereRadius;
    float discriminant = b * b - 4.0 * c;
    
    if (discriminant < 0.0) {
        return vec3(0.0);
    }
    
    float tNear = (-b - sqrt(discriminant)) * 0.5;
    float tFar = (-b + sqrt(discriminant)) * 0.5;
    
    // If we're outside the atmosphere, adjust entry point
    if (cameraHeight > uAtmosphereDepth) {
        tNear = max(tNear, 0.0);
    }
    
    // Sample along the ray
    vec3 rayleighSum = vec3(0.0);
    vec3 mieSum = vec3(0.0);
    
    float stepSize = (tFar - tNear) / float(SAMPLE_COUNT);
    
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        float t = tNear + stepSize * (float(i) + 0.5);
        vec3 samplePos = cameraPos + rayDir * t;
        float sampleHeight = length(samplePos) - planetRadius;
        
        if (sampleHeight < 0.0 || sampleHeight > uAtmosphereDepth) {
            continue;
        }
        
        // Calculate density at sample point (exponential falloff)
        float density = exp(-sampleHeight / 8000.0);
        
        // Calculate optical depth to sun from sample point
        vec3 toSun = sunDir;
        float sunB = 2.0 * dot(samplePos, toSun);
        float sunC = dot(samplePos, samplePos) - planetRadius * planetRadius;
        float sunDiscriminant = sunB * sunB - 4.0 * sunC;
        
        float sunDistance = 0.0;
        if (sunDiscriminant >= 0.0) {
            float sunT = (-sunB - sqrt(sunDiscriminant)) * 0.5;
            sunDistance = max(sunT, 0.0);
        }
        
        // Calculate transmittance (light attenuation through atmosphere)
        float rayleighOpticalDepth = 0.0;
        float mieOpticalDepth = 0.0;
        
        // Simplified: integrate density along path to sun
        float sunStepSize = sunDistance / 8.0;
        for (int j = 0; j < 8; j++) {
            float sunT = sunStepSize * (float(j) + 0.5);
            vec3 sunSamplePos = samplePos + toSun * sunT;
            float sunHeight = length(sunSamplePos) - planetRadius;
            
            if (sunHeight > 0.0) {
                float sunDensity = exp(-sunHeight / 8000.0);
                rayleighOpticalDepth += sunDensity * sunStepSize;
                mieOpticalDepth += sunDensity * sunStepSize;
            }
        }
        
        // Add optical depth from camera to sample point
        float cameraToSampleDepth = t * density;
        rayleighOpticalDepth += cameraToSampleDepth;
        mieOpticalDepth += cameraToSampleDepth;
        
        // Calculate scattering coefficients
        vec3 rayleighAttenuation = exp(-uRayleighColor * rayleighOpticalDepth);
        vec3 mieAttenuation = exp(-uMieColor * mieOpticalDepth);
        
        // Phase functions
        float cosTheta = dot(rayDir, sunDir);
        float rayleighPhaseValue = RayleighPhase(cosTheta);
        float miePhaseValue = MiePhase(cosTheta, uMieAnisotropy);
        
        // Accumulate scattered light
        rayleighSum += rayleighAttenuation * rayleighPhaseValue * density * stepSize;
        mieSum += mieAttenuation * miePhaseValue * density * stepSize;
    }
    
    // Apply sun intensity and combine
    vec3 result = (rayleighSum * uRayleighColor + mieSum * uMieColor) * uSunIntensity;
    
    return result;
}

void main() {
    vec3 rayDirection = normalize(vWorldPos);
    vec3 scatteredLight = CalculateScattering(rayDirection, uSunDirection);
    
    // Tone mapping (Reinhard)
    scatteredLight = scatteredLight / (scatteredLight + vec3(1.0));
    
    // Gamma correction
    scatteredLight = pow(scatteredLight, vec3(1.0/2.2));
    
    FragColor = vec4(scatteredLight, 1.0);
}
)";

namespace {
    std::string LoadShaderSource(const char* source) {
        return std::string(source);
    }

    unsigned int CompileShader(unsigned int type, const std::string& source) {
        unsigned int id = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        int success;
        char infoLog[512];
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(id, 512, nullptr, infoLog);
            std::cerr << "Shader compilation error: " << infoLog << std::endl;
            return 0;
        }
        return id;
    }

    unsigned int CreateProgram(unsigned int vertShader, unsigned int fragShader) {
        unsigned int program = glCreateProgram();
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);

        int success;
        char infoLog[512];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Shader program linking error: " << infoLog << std::endl;
            return 0;
        }
        return program;
    }
}

SkyAtmosphere::SkyAtmosphere() 
    : m_vao(0), m_vbo(0), m_shaderProgram(0)
    , m_locViewProj(-1), m_locSunDirection(-1)
    , m_locRayleighColor(-1), m_locMieColor(-1)
    , m_locSunIntensity(-1), m_locPlanetRadius(-1)
    , m_locAtmosphereDepth(-1), m_locMieAnisotropy(-1) {
}

SkyAtmosphere::~SkyAtmosphere() {
    Cleanup();
}

void SkyAtmosphere::Initialize() {
    CreateGeometry();
    LoadShaders();
    CalculateScatteringCoefficients();
}

void SkyAtmosphere::CreateGeometry() {
    // Create a sphere for the sky dome
    const int segments = 64;
    const int rings = 32;
    
    std::vector<float> vertices;
    
    for (int i = 0; i <= rings; ++i) {
        float theta = i * float(PI) / rings;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int j = 0; j <= segments; ++j) {
            float phi = j * 2.0f * float(PI) / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }
    
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void SkyAtmosphere::LoadShaders() {
    unsigned int vertShader = CompileShader(GL_VERTEX_SHADER, LoadShaderSource(skyVertexShaderSource));
    unsigned int fragShader = CompileShader(GL_FRAGMENT_SHADER, LoadShaderSource(skyFragmentShaderSource));
    
    if (vertShader && fragShader) {
        m_shaderProgram = CreateProgram(vertShader, fragShader);
    }
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    // Get uniform locations
    if (m_shaderProgram) {
        m_locViewProj = glGetUniformLocation(m_shaderProgram, "uViewProj");
        m_locSunDirection = glGetUniformLocation(m_shaderProgram, "uSunDirection");
        m_locRayleighColor = glGetUniformLocation(m_shaderProgram, "uRayleighColor");
        m_locMieColor = glGetUniformLocation(m_shaderProgram, "uMieColor");
        m_locSunIntensity = glGetUniformLocation(m_shaderProgram, "uSunIntensity");
        m_locPlanetRadius = glGetUniformLocation(m_shaderProgram, "uPlanetRadius");
        m_locAtmosphereDepth = glGetUniformLocation(m_shaderProgram, "uAtmosphereDepth");
        m_locMieAnisotropy = glGetUniformLocation(m_shaderProgram, "uMieAnisotropy");
    }
}

void SkyAtmosphere::CalculateScatteringCoefficients() {
    // Convert scattering coefficients to colors
    // These are already set in the config, but we could compute them here
    // based on wavelength spectra if needed
}

void SkyAtmosphere::SetConfig(const AtmosphereConfig& config) {
    m_config = config;
}

void SkyAtmosphere::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& sunDirection) {
    if (!m_shaderProgram) {
        return;
    }
    
    glUseProgram(m_shaderProgram);
    
    // Set uniforms
    glm::mat4 viewProj = projectionMatrix * glm::mat4(glm::mat3(viewMatrix));
    glUniformMatrix4fv(m_locViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform3f(m_locSunDirection, sunDirection.x, sunDirection.y, sunDirection.z);
    glUniform3f(m_locRayleighColor, m_config.rayleighScattering.r, m_config.rayleighScattering.g, m_config.rayleighScattering.b);
    glUniform3f(m_locMieColor, m_config.mieScattering, m_config.mieScattering, m_config.mieScattering);
    glUniform1f(m_locSunIntensity, m_config.sunIntensity);
    glUniform1f(m_locPlanetRadius, m_config.planetRadius);
    glUniform1f(m_locAtmosphereDepth, m_config.atmosphereDepth);
    glUniform1f(m_locMieAnisotropy, m_config.mieAnisotropy);
    
    // Render sky dome
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 66 * 33); // (segments+1) * (rings+1)
    glBindVertexArray(0);
    
    glUseProgram(0);
}

void SkyAtmosphere::Cleanup() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
}
