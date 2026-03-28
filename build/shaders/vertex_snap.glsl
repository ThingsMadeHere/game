#version 330 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec4 vertexColor;

out vec3 fragNormal;
out vec4 fragColor;
out vec4 fragPosLightSpace;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 worldPos = model * vec4(vertexPosition, 1.0);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    
    // Transform position for shadow mapping
    fragPosLightSpace = lightSpaceMatrix * worldPos;
    
    // Transform normal to world space
    fragNormal = mat3(transpose(inverse(model))) * vertexNormal;
    fragColor = vertexColor;
}
