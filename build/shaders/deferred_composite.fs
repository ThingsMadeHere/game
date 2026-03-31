#version 330 core

in vec2 texCoord;

out vec4 fragColor;

// G-Buffer inputs (populated by geometry pass)
uniform sampler2D uColorTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uDepthTexture;

// Lighting parameters
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uViewPos;

// Matrices for position reconstruction
uniform mat4 uInvProjection;
uniform mat4 uInvView;
uniform vec2 uScreenSize;

// Reconstruct world position from UV and depth
vec3 ReconstructWorldPosition(vec2 uv, float depth) {
    // Convert UV to NDC [-1, 1]
    vec4 clipPos = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    
    // Transform to view space using inverse projection
    vec4 viewPos = uInvProjection * clipPos;
    viewPos.xyz /= viewPos.w;
    viewPos.z = -depth; // Use linear depth for view space Z
    viewPos.w = 1.0;
    
    // Transform to world space using inverse view
    vec4 worldPos = uInvView * viewPos;
    return worldPos.xyz;
}

void main() {
    // Sample G-Buffer
    vec4 color = texture(uColorTexture, texCoord);
    vec3 normal = texture(uNormalTexture, texCoord).xyz;
    float depth = texture(uDepthTexture, texCoord).r;
    
    // Discard if no geometry (background)
    if (depth <= 0.0) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Reconstruct world position
    vec3 worldPos = ReconstructWorldPosition(texCoord, depth);
    
    // Calculate lighting
    vec3 N = normalize(normal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uViewPos - worldPos);
    
    // Diffuse (Lambert)
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = NdotL * uLightColor;
    
    // Specular (Blinn-Phong)
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float specular = pow(NdotH, 32.0) * 0.5;
    
    // Combine lighting
    vec3 lighting = uAmbientColor + diffuse + vec3(specular);
    
    // Apply lighting to albedo
    vec3 finalColor = color.rgb * lighting;
    
    // Simple fog based on depth
    float fogStart = 50.0;
    float fogEnd = 200.0;
    float fogFactor = clamp((depth - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 fogColor = vec3(0.5, 0.6, 0.8); // Sky blue fog
    
    finalColor = mix(finalColor, fogColor, fogFactor);
    
    fragColor = vec4(finalColor, color.a);
}
