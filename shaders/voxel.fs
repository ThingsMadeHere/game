#version 330 core

in vec3 fragPos;
in vec2 texCoord;
in vec3 normal;

out vec4 fragColor;

uniform sampler3D densityTexture; // 3D density texture (black=air, white=block)
uniform vec3 chunkPosition;      // World position of this chunk
uniform float voxelSize;          // Size of each voxel

void main() {
    // Convert fragment position to local chunk coordinates
    vec3 localPos = fragPos - chunkPosition;
    
    // Convert to texture coordinates (0-1 range)
    vec3 texCoord = localPos / (16.0 * voxelSize);
    
    // Sample the 3D density texture
    float density = texture(densityTexture, texCoord).r;
    
    // If density is 0 (air), discard this fragment
    if (density < 0.5) {
        discard;
    }
    
    // Simple lighting based on normal
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    float lighting = max(dot(normal, lightDir), 0.3);
    
    // Base color with lighting
    vec3 baseColor = vec3(0.5, 0.7, 0.3); // Green terrain
    fragColor = vec4(baseColor * lighting, 1.0);
}
