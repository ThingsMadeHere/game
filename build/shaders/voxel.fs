#version 330 core

in vec3 fragPos;
in vec2 texCoord;
in vec3 normal;

out vec4 fragColor;

uniform sampler2D grassTexture;    // Grass texture (repeating 32x32)
uniform vec3 chunkPosition;        // World position of this chunk
uniform float voxelSize;           // Size of each voxel

// Lighting uniforms
uniform vec3 lightDir;             // Directional light direction
uniform vec3 lightColor;           // Light color
uniform vec3 ambientColor;         // Ambient light color
uniform vec3 viewPos;              // Camera position for specular

void main() {
    // DEBUG: Output bright magenta to verify shader is running
    fragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
