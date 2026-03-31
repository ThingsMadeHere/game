#version 330 core

in vec2 texCoord;

out vec4 fragColor;

// G-Buffer inputs (populated by geometry pass)
uniform sampler2D uColorTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uDepthTexture;

// Shadow map
uniform sampler2D uShadowMap;
uniform mat4 uLightSpaceMatrix;
uniform float uShadowBias;

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

// Simple PCF shadow sampling
float CalculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if outside shadow map
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0) {
        return 0.0; // No shadow outside frustum
    }
    // Check if behind shadow map near plane
    if (projCoords.z < 0.0) {
        return 0.0;
    }
    
    // Get current depth
    float currentDepth = projCoords.z;
    
    // Calculate bias based on surface angle - smaller bias for better contact shadows
    float bias = max(uShadowBias * (1.0 - dot(normal, lightDir)), 0.001);
    
    // Sample shadow map with PCF (3x3 kernel)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

void main() {
    // Sample G-Buffer
    vec4 albedo = texture(uColorTexture, texCoord);
    vec3 normal = normalize(texture(uNormalTexture, texCoord).xyz);
    float depth = texture(uDepthTexture, texCoord).r;
    
    // Discard if no geometry (background)
    if (depth <= 0.0) {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Reconstruct world position from depth
    vec3 worldPos = ReconstructWorldPosition(texCoord, depth);
    
    // Calculate light direction (assuming directional light)
    vec3 lightDir = normalize(-uLightDir);
    
    // Calculate view direction
    vec3 viewDir = normalize(uViewPos - worldPos);
    
    // Ambient lighting
    vec3 ambient = uAmbientColor * albedo.rgb;
    
    // Diffuse lighting (Lambertian)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * albedo.rgb;
    
    // Specular lighting (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specAngle = max(dot(normal, halfwayDir), 0.0);
    float specular = pow(specAngle, 32.0); // Shininess factor
    vec3 specularColor = specular * uLightColor;
    
    // Combine lighting components
    vec3 result = ambient + diffuse + specularColor;
    
    fragColor = vec4(result, albedo.a);
}
