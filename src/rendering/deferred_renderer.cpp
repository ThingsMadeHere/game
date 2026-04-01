#include "deferred_renderer.h"
#include <cstdio>
#include <cstring>
#include <raymath.h>

namespace {
    constexpr float QUAD_VERTICES[] = {
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
    };
    
    constexpr int POSITION_ATTRIB = 0;
    constexpr int TEXCOORD_ATTRIB = 1;
    constexpr int VERTEX_COMPONENTS = 2;
    constexpr int TEXCOORD_COMPONENTS = 2;
    constexpr int STRIDE_FLOATS = 4;
}

DeferredRenderer::DeferredRenderer() 
    : lightSpaceMatrix(MatrixIdentity()) {
}

DeferredRenderer::~DeferredRenderer() {
    Cleanup();
}

bool DeferredRenderer::Init(int width, int height) {
    if (initialized) {
        Cleanup();
    }
    
    screenWidth = width;
    screenHeight = height;
    
    geometryShader = LoadShader("shaders/deferred_geometry.vs", "shaders/deferred_geometry.fs");
    if (geometryShader.id == 0) {
        fprintf(stderr, "Failed to load deferred geometry shader!\n");
        return false;
    }
    
    compositeShader = LoadShader("shaders/deferred_composite.vs", "shaders/deferred_composite.fs");
    if (compositeShader.id == 0) {
        fprintf(stderr, "Failed to load deferred composite shader!\n");
        UnloadShader(geometryShader);
        return false;
    }
    
    compositeUniforms.colorTexture = GetShaderLocation(compositeShader, "uColorTexture");
    compositeUniforms.normalTexture = GetShaderLocation(compositeShader, "uNormalTexture");
    compositeUniforms.depthTexture = GetShaderLocation(compositeShader, "uDepthTexture");
    compositeUniforms.lightDir = GetShaderLocation(compositeShader, "uLightDir");
    compositeUniforms.lightColor = GetShaderLocation(compositeShader, "uLightColor");
    compositeUniforms.ambientColor = GetShaderLocation(compositeShader, "uAmbientColor");
    compositeUniforms.viewPos = GetShaderLocation(compositeShader, "uViewPos");
    compositeUniforms.invProjection = GetShaderLocation(compositeShader, "uInvProjection");
    compositeUniforms.invView = GetShaderLocation(compositeShader, "uInvView");
    compositeUniforms.screenSize = GetShaderLocation(compositeShader, "uScreenSize");
    compositeUniforms.shadowMap = GetShaderLocation(compositeShader, "uShadowMap");
    compositeUniforms.lightSpaceMatrix = GetShaderLocation(compositeShader, "uLightSpaceMatrix");
    compositeUniforms.shadowBias = GetShaderLocation(compositeShader, "uShadowBias");
    
    shadowShader = LoadShader("shaders/shadow.vs", "shaders/shadow.fs");
    if (shadowShader.id == 0) {
        fprintf(stderr, "Failed to load shadow shader!\n");
        UnloadShader(geometryShader);
        UnloadShader(compositeShader);
        return false;
    }
    
    debugShader = LoadShader("shaders/debug_visualize.vs", "shaders/debug_visualize.fs");
    if (debugShader.id == 0) {
        fprintf(stderr, "Failed to load debug visualize shader!\n");
        UnloadShader(geometryShader);
        UnloadShader(compositeShader);
        UnloadShader(shadowShader);
        return false;
    }
    
    if (!CreateShadowMap()) {
        fprintf(stderr, "Failed to create shadow map!\n");
        UnloadShader(geometryShader);
        UnloadShader(compositeShader);
        UnloadShader(shadowShader);
        return false;
    }
    
    if (!CreateGBuffer(width, height)) {
        fprintf(stderr, "Failed to create G-Buffer!\n");
        DeleteShadowMapResources();
        UnloadShader(geometryShader);
        UnloadShader(compositeShader);
        UnloadShader(shadowShader);
        return false;
    }
    
    CreateFullscreenQuad();
    
    initialized = true;
    fprintf(stderr, "Deferred Renderer initialized (%dx%d)\n", width, height);
    return true;
}

void DeferredRenderer::Cleanup() {
    if (!initialized) return;
    
    DeleteGBufferResources();
    DeleteShadowMapResources();
    
    if (quadVAO != 0) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
    if (quadVBO != 0) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
    if (geometryShader.id != 0) { UnloadShader(geometryShader); geometryShader = {}; }
    if (compositeShader.id != 0) { UnloadShader(compositeShader); compositeShader = {}; }
    if (shadowShader.id != 0) { UnloadShader(shadowShader); shadowShader = {}; }
    if (debugShader.id != 0) { UnloadShader(debugShader); debugShader = {}; }
    
    initialized = false;
}

void DeferredRenderer::BeginGeometryPass(const Camera3D& camera) {
    if (!initialized) return;
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    BeginShaderMode(geometryShader);
}

void DeferredRenderer::EndGeometryPass() {
    EndShaderMode();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeferredRenderer::BeginShadowPass(const Vector3& lightDir, const Vector3& sceneCenter, float sceneRadius) {
    if (!initialized) return;
    
    Vector3 lightPos = Vector3Add(sceneCenter, Vector3Scale(Vector3Normalize(lightDir), -sceneRadius * LIGHT_DISTANCE_FACTOR));
    Matrix lightView = MatrixLookAt(lightPos, sceneCenter, (Vector3){0.0f, 1.0f, 0.0f});
    
    float orthoSize = sceneRadius * ORTHO_SCALE_FACTOR;
    Matrix lightProjection = MatrixOrtho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, sceneRadius * 4.0f);
    
    lightSpaceMatrix = MatrixMultiply(lightView, lightProjection);
    
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}

void DeferredRenderer::EndShadowPass() {
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void DeferredRenderer::BeginCompositePass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
}

void DeferredRenderer::EndCompositePass() {
    glEnable(GL_DEPTH_TEST);
}

void DeferredRenderer::SetupCompositeShaderTextures(bool useShadows) {
    int texUnit0 = 0, texUnit1 = 1, texUnit2 = 2;
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    SetShaderValue(compositeShader, compositeUniforms.colorTexture, &texUnit0, SHADER_UNIFORM_INT);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    SetShaderValue(compositeShader, compositeUniforms.normalTexture, &texUnit1, SHADER_UNIFORM_INT);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    SetShaderValue(compositeShader, compositeUniforms.depthTexture, &texUnit2, SHADER_UNIFORM_INT);
    
    if (useShadows) {
        int texUnit3 = 3;
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
        SetShaderValue(compositeShader, compositeUniforms.shadowMap, &texUnit3, SHADER_UNIFORM_INT);
    }
}

void DeferredRenderer::SetupCompositeShaderLighting(const Camera3D& camera, const Vector3& lightDir,
                                                     const Vector3& lightColor, const Vector3& ambientColor) {
    SetShaderValue(compositeShader, compositeUniforms.lightDir, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(compositeShader, compositeUniforms.lightColor, &lightColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(compositeShader, compositeUniforms.ambientColor, &ambientColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(compositeShader, compositeUniforms.viewPos, &camera.position, SHADER_UNIFORM_VEC3);
    
    float shadowBias = DEFAULT_SHADOW_BIAS;
    SetShaderValue(compositeShader, compositeUniforms.shadowBias, &shadowBias, SHADER_UNIFORM_FLOAT);
    SetShaderValueMatrix(compositeShader, compositeUniforms.lightSpaceMatrix, lightSpaceMatrix);
    
    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
    float aspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.01f, 1000.0f);
    
    Matrix invView = MatrixInvert(view);
    Matrix invProjection = MatrixInvert(projection);
    
    SetShaderValueMatrix(compositeShader, compositeUniforms.invView, invView);
    SetShaderValueMatrix(compositeShader, compositeUniforms.invProjection, invProjection);
    
    float screenSize[2] = { static_cast<float>(screenWidth), static_cast<float>(screenHeight) };
    SetShaderValue(compositeShader, compositeUniforms.screenSize, screenSize, SHADER_UNIFORM_VEC2);
}

void DeferredRenderer::Composite(const Camera3D& camera, const Vector3& lightDir, 
                                  const Vector3& lightColor, const Vector3& ambientColor) {
    if (!initialized) return;
    
    BeginShaderMode(compositeShader);
    SetupCompositeShaderTextures(false);
    SetupCompositeShaderLighting(camera, lightDir, lightColor, ambientColor);
    DrawFullscreenQuad();
    glBindTexture(GL_TEXTURE_2D, 0);
    EndShaderMode();
}

void DeferredRenderer::CompositeWithShadows(const Camera3D& camera, const Vector3& lightDir, 
                                           const Vector3& lightColor, const Vector3& ambientColor) {
    if (!initialized) return;
    
    BeginShaderMode(compositeShader);
    SetupCompositeShaderTextures(true);
    SetupCompositeShaderLighting(camera, lightDir, lightColor, ambientColor);
    DrawFullscreenQuad();
    glBindTexture(GL_TEXTURE_2D, 0);
    EndShaderMode();
}

void DeferredRenderer::DebugDrawGBufferColor() {
    if (!initialized) return;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    DrawFullscreenQuad();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DeferredRenderer::DebugDrawGBufferNormals() {
    if (!initialized) return;
    
    // Use debug shader to remap [-1,1] normals to [0,1] for visualization
    BeginShaderMode(debugShader);
    
    int texLoc = GetShaderLocation(debugShader, "uTexture");
    int scaleLoc = GetShaderLocation(debugShader, "uScale");
    int offsetLoc = GetShaderLocation(debugShader, "uOffset");
    
    float scale = 0.5f;   // Remap [-1,1] to [0,1]: value * 0.5 + 0.5
    float offset = 0.5f;
    
    SetShaderValue(debugShader, scaleLoc, &scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(debugShader, offsetLoc, &offset, SHADER_UNIFORM_FLOAT);
    
    int texUnit = 0;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    SetShaderValue(debugShader, texLoc, &texUnit, SHADER_UNIFORM_INT);
    
    DrawFullscreenQuad();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    EndShaderMode();
}

void DeferredRenderer::DrawFullscreenQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void DeferredRenderer::Resize(int width, int height) {
    if (!initialized) return;
    screenWidth = width;
    screenHeight = height;
    DeleteGBufferResources();
    CreateGBuffer(width, height);
}

void DeferredRenderer::CreateFullscreenQuad() {
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTICES), QUAD_VERTICES, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(POSITION_ATTRIB);
    glVertexAttribPointer(POSITION_ATTRIB, VERTEX_COMPONENTS, GL_FLOAT, GL_FALSE, 
                          STRIDE_FLOATS * sizeof(float), (void*)0);
    
    glEnableVertexAttribArray(TEXCOORD_ATTRIB);
    glVertexAttribPointer(TEXCOORD_ATTRIB, TEXCOORD_COMPONENTS, GL_FLOAT, GL_FALSE, 
                          STRIDE_FLOATS * sizeof(float), (void*)(VERTEX_COMPONENTS * sizeof(float)));
    
    glBindVertexArray(0);
}

bool DeferredRenderer::CreateGBuffer(int width, int height) {
    glGenFramebuffers(1, &gBufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
    
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    
    glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTexture, 0);
    
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, depthTexture, 0);
    
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    
    GLenum drawBuffers[COLOR_ATTACHMENT_COUNT] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(COLOR_ATTACHMENT_COUNT, drawBuffers);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer incomplete!\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool DeferredRenderer::CreateShadowMap() {
    glGenFramebuffers(1, &shadowMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    
    glGenTextures(1, &shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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
    fprintf(stderr, "Shadow map created (%dx%d)\n", shadowMapSize, shadowMapSize);
    return true;
}

void DeferredRenderer::DeleteGBufferResources() {
    if (gBufferFBO != 0) { glDeleteFramebuffers(1, &gBufferFBO); gBufferFBO = 0; }
    if (colorTexture != 0) { glDeleteTextures(1, &colorTexture); colorTexture = 0; }
    if (normalTexture != 0) { glDeleteTextures(1, &normalTexture); normalTexture = 0; }
    if (depthTexture != 0) { glDeleteTextures(1, &depthTexture); depthTexture = 0; }
    if (depthRenderbuffer != 0) { glDeleteRenderbuffers(1, &depthRenderbuffer); depthRenderbuffer = 0; }
}

void DeferredRenderer::DeleteShadowMapResources() {
    if (shadowMapFBO != 0) { glDeleteFramebuffers(1, &shadowMapFBO); shadowMapFBO = 0; }
    if (shadowMapTexture != 0) { glDeleteTextures(1, &shadowMapTexture); shadowMapTexture = 0; }
}
