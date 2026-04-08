#version 330

// Input vertex attributes (from vertex shader)
in vec3 vPosition;

// Input uniform values
uniform vec3 uSunPos;
uniform float uSunIntensity;
uniform float uPlanetRadius;
uniform float uAtmosphereRadius;
uniform vec3 uRayleighScattering;
uniform float uMieScattering;
uniform float uRayleighScaleHeight;
uniform float uMieScaleHeight;
uniform float uMieG;

// Output fragment color
out vec4 fragColor;

// Rayleigh scattering phase function
float rayleighPhase(float cosTheta) {
    return 3.0 / (16.0 * 3.14159265) * (1.0 + cosTheta * cosTheta);
}

// Mie scattering phase function
float miePhase(float cosTheta, float g) {
    float g2 = g * g;
    return (3.0 / (8.0 * 3.14159265)) * ((1.0 - g2) * (1.0 + cosTheta * cosTheta)) / 
           pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
}

// Main atmosphere calculation
vec3 atmosphere(vec3 rayDir, vec3 rayStart, vec3 sunPos) {
    // Normalize directions
    rayDir = normalize(rayDir);
    sunPos = normalize(sunPos);
    
    // Calculate scattering based on viewing angle
    float cosTheta = dot(rayDir, sunPos);
    
    // Simple height-based calculation
    float height = max(0.0, rayDir.y); // y component gives height angle
    
    // Rayleigh scattering (stronger at zenith)
    float rayleighFactor = height * 0.8 + 0.2;
    vec3 rayleighColor = uRayleighScattering * rayleighPhase(cosTheta) * rayleighFactor;
    
    // Mie scattering (stronger near horizon)
    float mieFactor = (1.0 - height) * 0.5;
    vec3 mieColor = vec3(uMieScattering) * miePhase(cosTheta, uMieG) * mieFactor;
    
    // Combine scattering
    vec3 color = rayleighColor + mieColor;
    color *= uSunIntensity;
    
    // Apply exposure
    color = 1.0 - exp(-1.0 * color);
    
    return color;
}

void main()
{
    // Calculate sky color using Rayleigh scattering
    vec3 rayDir = normalize(vPosition);
    vec3 rayStart = vec3(0.0, uPlanetRadius + 1000.0, 0.0); // Start slightly above surface
    
    vec3 color = atmosphere(rayDir, rayStart, uSunPos);
    
    // Add some base color to ensure visibility
    color += vec3(0.1, 0.2, 0.4);
    
    // Output final color
    fragColor = vec4(color, 1.0);
}
