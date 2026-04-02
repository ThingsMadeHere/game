#include "shadow_renderer.h"
#include <cstdio>
#include <cstring>
#include <raymath.h>

ShadowRenderer::ShadowRenderer() {}

ShadowRenderer::~ShadowRenderer() {
    Cleanup();
}

bool ShadowRenderer::Init(int size) {
    if (initialized) Cleanup();
    
    shadowMapSize = size;
    
    if (!CreateShadowMap(size)) {
        fprintf(stderr, "Failed to create shadow map!\n");
        return false;
    }
    
    if (!LoadShaders()) {
        fprintf(stderr, "Failed to load shaders!\n");
        Cleanup();
        return false;
    }
    
    initialized = true;
    fprintf(stderr, "Shadow Renderer initialized (%dx%d)\n", size, size);
    return true;
}

void ShadowRenderer::Cleanup() {
    if (shadowMapFBO != 0) {
        glDeleteFramebuffers(1, &shadowMapFBO);
        shadowMapFBO = 0;
    }
    if (shadowMapTexture != 0) {
        glDeleteTextures(1, &shadowMapTexture);
        shadowMapTexture = 0;
    }
    if (shadowShader.id != 0) {
        UnloadShader(shadowShader);
        shadowShader = {0};
    }
    if (mainShader.id != 0) {
        UnloadShader(mainShader);
        mainShader = {0};
    }
    initialized = false;
}

bool ShadowRenderer::CreateShadowMap(int size) {
    glGenFramebuffers(1, &shadowMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    
    glGenTextures(1, &shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Shadow map framebuffer incomplete!\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool ShadowRenderer::LoadShaders() {
    // Simple depth shader for shadow pass
    const char* shadowVs = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 lightSpaceMatrix;
uniform mat4 model;
void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";

    const char* shadowFs = R"(
#version 330 core
void main() {
    // Depth is written automatically
}
)";

    // Main shader with shadow sampling
    const char* mainVs = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

    const char* mainFs = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D shadowMap;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 viewPos;
uniform float shadowBias;

float ShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0) return 0.0;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    float bias = max(0.005 * (1.0 - dot(normalize(Normal), -lightDir)), shadowBias);
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    return shadow;
}

void main() {
    vec3 normal = normalize(Normal);
    vec3 lightDirection = normalize(-lightDir);
    
    float diff = max(dot(normal, lightDirection), 0.0);
    
    float shadow = ShadowCalculation(FragPosLightSpace);
    vec3 lighting = ambientColor + (1.0 - shadow) * diff * lightColor;
    
    // Simple grass color based on height/normal
    vec3 grassColor = vec3(0.2, 0.6, 0.2);
    vec3 dirtColor = vec3(0.4, 0.3, 0.2);
    vec3 color = mix(dirtColor, grassColor, max(dot(normal, vec3(0,1,0)), 0.0));
    
    FragColor = vec4(color * lighting, 1.0);
}
)";

    shadowShader = LoadShaderFromMemory(shadowVs, shadowFs);
    if (shadowShader.id == 0) {
        fprintf(stderr, "Failed to compile shadow shader!\n");
        return false;
    }
    
    mainShader = LoadShaderFromMemory(mainVs, mainFs);
    if (mainShader.id == 0) {
        fprintf(stderr, "Failed to compile main shader!\n");
        UnloadShader(shadowShader);
        return false;
    }
    
    // Get uniform locations
    shadowShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shadowShader, "model");
    shadowShader.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(shadowShader, "lightSpaceMatrix");
    
    mainShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(mainShader, "model");
    mainShader.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(mainShader, "view");
    mainShader.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(mainShader, "projection");
    
    return true;
}

void ShadowRenderer::BeginShadowPass(const Vector3& lightDir, const Vector3& sceneCenter, float sceneRadius) {
    if (!initialized) return;
    
    currentLightDir = lightDir;
    
    // Calculate light view matrix
    Vector3 lightPos = Vector3Add(sceneCenter, Vector3Scale(Vector3Normalize(lightDir), -sceneRadius * 2.0f));
    Matrix lightView = MatrixLookAt(lightPos, sceneCenter, (Vector3){0.0f, 1.0f, 0.0f});
    
    // Orthographic projection for directional light
    float orthoSize = sceneRadius * 1.5f;
    Matrix lightProjection = MatrixOrtho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, sceneRadius * 4.0f);
    
    lightSpaceMatrix = MatrixMultiply(lightView, lightProjection);
    
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}

void ShadowRenderer::EndShadowPass() {
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, GetScreenWidth(), GetScreenHeight());
}

void ShadowRenderer::BeginMainPass(const Camera3D& camera, const Vector3& lightDir) {
    if (!initialized) return;
    
    BeginShaderMode(mainShader);
    
    // Set matrices
    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.01f, 1000.0f);
    
    SetShaderValueMatrix(mainShader, mainShader.locs[SHADER_LOC_MATRIX_VIEW], view);
    SetShaderValueMatrix(mainShader, mainShader.locs[SHADER_LOC_MATRIX_PROJECTION], projection);
    
    // Set lighting uniforms
    int lightDirLoc = GetShaderLocation(mainShader, "lightDir");
    int lightColorLoc = GetShaderLocation(mainShader, "lightColor");
    int ambientLoc = GetShaderLocation(mainShader, "ambientColor");
    int viewPosLoc = GetShaderLocation(mainShader, "viewPos");
    int biasLoc = GetShaderLocation(mainShader, "shadowBias");
    int lightSpaceLoc = GetShaderLocation(mainShader, "lightSpaceMatrix");
    int shadowMapLoc = GetShaderLocation(mainShader, "shadowMap");
    
    Vector3 lightColor = {1.0f, 1.0f, 0.9f};
    Vector3 ambientColor = {0.3f, 0.35f, 0.4f};
    float bias = 0.005f;
    int texUnit = 0;
    
    SetShaderValue(mainShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(mainShader, lightColorLoc, &lightColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(mainShader, ambientLoc, &ambientColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(mainShader, viewPosLoc, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(mainShader, biasLoc, &bias, SHADER_UNIFORM_FLOAT);
    SetShaderValueMatrix(mainShader, lightSpaceLoc, lightSpaceMatrix);
    SetShaderValue(mainShader, shadowMapLoc, &texUnit, SHADER_UNIFORM_INT);
    
    // Bind shadow map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
}

void ShadowRenderer::EndMainPass() {
    EndShaderMode();
    glBindTexture(GL_TEXTURE_2D, 0);
}
