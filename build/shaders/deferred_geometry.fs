#version 330 core

in vec3 fragPos;
in vec2 texCoord;
in vec3 normal;

// G-Buffer outputs
layout (location = 0) out vec4 gColor;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out float gDepth;

uniform sampler2D colorTexture;
uniform vec3 baseColor;
uniform int useTexture;

// For linear depth calculation
uniform float nearPlane;
uniform float farPlane;

void main() {
    // Output albedo/color
    if (useTexture > 0) {
        gColor = texture(colorTexture, texCoord);
    } else {
        gColor = vec4(baseColor, 1.0);
    }
    
    // Output world-space normal (normalized)
    gNormal = vec4(normalize(normal), 1.0);
    
    // Output linear depth
    // Calculate linear depth from view space Z
    float linearDepth = length(fragPos); // Distance from camera
    gDepth = linearDepth;
}
