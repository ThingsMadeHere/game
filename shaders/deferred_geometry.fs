#version 330 core

in vec3 fragPos;
in vec2 texCoord;
in vec3 normal;

// G-Buffer outputs
layout (location = 0) out vec4 gColor;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out float gDepth;

// Raylib binds material texture to unit 0 by default
uniform sampler2D texture0;

void main() {
    // Sample albedo from texture
    vec4 albedo = texture(texture0, texCoord);
    
    // Output albedo to G-Buffer
    gColor = albedo;
    
    // Output normal to G-Buffer
    gNormal = vec4(normalize(normal), 1.0);
    
    // Output linear depth
    float linearDepth = length(fragPos);
    gDepth = linearDepth;
}
