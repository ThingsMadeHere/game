#version 330 core

in vec3 fragNormal;
in vec4 fragColor;
in vec4 fragPosLightSpace;

out vec4 finalColor;

uniform sampler2D shadowMap;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 ambientColor;
uniform int shadowMapResolution;

float computeShadow() {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0) return 1.0;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    float bias = max(0.005 * (1.0 - dot(normalize(fragNormal), -sunDirection)), 0.001);
    
    return currentDepth - bias > closestDepth ? 0.5 : 1.0;
}

void main() {
    float shadow = computeShadow();
    float light = max(0.0, dot(normalize(fragNormal), -sunDirection));
    
    vec3 color = fragColor.rgb * (ambientColor + sunColor * light * shadow);
    finalColor = vec4(color, 1.0);
}
