#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform float uScale = 0.5;  // Scale for remapping
uniform float uOffset = 0.5; // Offset for remapping

void main() {
    vec3 value = texture(uTexture, TexCoords).rgb;
    // Remap from [-1,1] to [0,1] for normals, or pass through for color
    vec3 remapped = value * uScale + uOffset;
    FragColor = vec4(remapped, 1.0);
}
