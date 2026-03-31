#include "deferred_renderer.h"
#include <cstdio>
#include <cstring>
#include <raymath.h>

// Full screen quad vertices (position + texcoord)
static const float quadVertices[] = {
    // Positions    // TexCoords
    -1.0f,  1.0f,   0.0f, 1.0f,
    -1.0f, -1.0f,   0.0f, 0.0f,
     1.0f, -1.0f,   1.0f, 0.0f,
    -1.0f,  1.0f,   0.0f, 1.0f,
     1.0f, -1.0f,   1.0f, 0.0f,
     1.0f,  1.0f,   1.0f, 1.0f
};

DeferredRenderer::DeferredRenderer() 
    : fbo(0), colorTexture(0), normalTexture(0), depthTexture(0), depthBuffer(0),
      screenWidth(0), screenHeight(0), quadVAO(0), quadVBO(0),
      shadowMapFBO(0), shadowMapTexture(0), shadowMapSize(2048),
      initialized(false) {
    memset(&geometryShader, 0, sizeof(Shader));
    memset(&compositeShader, 0, sizeof(Shader));
    memset(&shadowShader, 0, sizeof(Shader));
    lightSpaceMatrix = MatrixIdentity();
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
    
    // Load shaders
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
    
    // Get composite shader uniform locations
    uColorTexture = GetShaderLocation(compositeShader, "uColorTexture");
    uNormalTexture = GetShaderLocation(compositeShader, "uNormalTexture");
    uDepthTexture = GetShaderLocation(compositeShader, "uDepthTexture");
    uLightDir = GetShaderLocation(compositeShader, "uLightDir");
    uLightColor = GetShaderLocation(compositeShader, "uLightColor");
    uAmbientColor = GetShaderLocation(compositeShader, "uAmbientColor");
    uViewPos = GetShaderLocation(compositeShader, "uViewPos");
    uInvProjection = GetShaderLocation(compositeShader, "uInvProjection");
    uInvView = GetShaderLocation(compositeShader, "uInvView");
    uScreenSize = GetShaderLocation(compositeShader, "uScreenSize");
    
    // Shadow map uniforms
    uShadowMap = GetShaderLocation(compositeShader, "uShadowMap");
    uLightSpaceMatrix = GetShaderLocation(compositeShader, "uLightSpaceMatrix");
    uShadowBias = GetShaderLocation(compositeShader, "uShadowBias");
    
    // Load shadow shader
    shadowShader = LoadShader("shaders/shadow.vs", "shaders/shadow.fs");
    if (shadowShader.id == 0) {
        fprintf(stderr, "Failed to load shadow shader!\n");
        UnloadShader(geometryShader);
        UnloadShader(compositeShader);
        return false;
    }
    
    // Create shadow map
    if (!CreateShadowMap()) {
        fprintf(stderr, "Failed to create shadow map!\n");
        UnloadShader(geometryShader);
        UnloadShader(compositeShader);
        UnloadShader(shadowShader);
        return false;
    }
    
    // Create G-Buffer
    if (!CreateGBuffer(width, height)) {
        fprintf(stderr, "Failed to create G-Buffer!\n");
        UnloadShader(geometryShader);
        UnloadShader(compositeShader);
        return false;
    }
    
    // Create full screen quad
    CreateFullscreenQuad();
    
    initialized = true;
    fprintf(stderr, "Deferred Renderer initialized (%dx%d)\n", width, height);
    return true;
}

bool DeferredRenderer::CreateGBuffer(int width, int height) {
    // Generate framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    // Color attachment (RGBA8)
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    
    // Normal attachment (RGB16F for precision)
    glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTexture, 0);
    
    // Depth attachment (R32F for linear depth)
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, depthTexture, 0);
    
    // Depth/stencil renderbuffer for depth testing
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    
    // Set draw buffers
    GLenum drawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, drawBuffers);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer incomplete!\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void DeferredRenderer::CreateFullscreenQuad() {
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // TexCoord attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

void DeferredRenderer::Cleanup() {
    if (fbo != 0) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (colorTexture != 0) {
        glDeleteTextures(1, &colorTexture);
        colorTexture = 0;
    }
    if (normalTexture != 0) {
        glDeleteTextures(1, &normalTexture);
        normalTexture = 0;
    }
    if (depthTexture != 0) {
        glDeleteTextures(1, &depthTexture);
        depthTexture = 0;
    }
    if (depthBuffer != 0) {
        glDeleteRenderbuffers(1, &depthBuffer);
        depthBuffer = 0;
    }
    if (shadowMapFBO != 0) {
        glDeleteFramebuffers(1, &shadowMapFBO);
        shadowMapFBO = 0;
    }
    if (shadowMapTexture != 0) {
        glDeleteTextures(1, &shadowMapTexture);
        shadowMapTexture = 0;
    }
    if (quadVAO != 0) {
        glDeleteVertexArrays(1, &quadVAO);
        quadVAO = 0;
    }
    if (quadVBO != 0) {
        glDeleteBuffers(1, &quadVBO);
        quadVBO = 0;
    }
    if (geometryShader.id != 0) {
        UnloadShader(geometryShader);
        geometryShader = {0};
    }
    if (compositeShader.id != 0) {
        UnloadShader(compositeShader);
        compositeShader = {0};
    }
    if (shadowShader.id != 0) {
        UnloadShader(shadowShader);
        shadowShader = {0};
    }
    initialized = false;
}

void DeferredRenderer::BeginGeometryPass(const Camera3D& camera) {
    if (!initialized) return;
    
    // Bind G-Buffer framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    // Clear all attachments
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing for geometry pass
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Activate geometry shader for G-Buffer output
    BeginShaderMode(geometryShader);
}

void DeferredRenderer::EndGeometryPass() {
    // End geometry shader mode
    EndShaderMode();
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeferredRenderer::BeginCompositePass() {
    // Default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Disable depth testing for fullscreen quad
    glDisable(GL_DEPTH_TEST);
}

void DeferredRenderer::EndCompositePass() {
    // Re-enable depth test for next frame
    glEnable(GL_DEPTH_TEST);
}

void DeferredRenderer::DebugDrawGBufferColor() {
    if (!initialized) return;
    
    // Just draw the color texture directly to screen without any shader processing
    // This shows the raw geometry pass output (normals as color in debug mode)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    
    // Use a simple texture draw with default shader (no lighting)
    DrawFullscreenQuad();
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DeferredRenderer::Composite(const Camera3D& camera, const Vector3& lightDir, 
                                  const Vector3& lightColor, const Vector3& ambientColor) {
    if (!initialized) return;
    
    // DEBUG: Just draw raw color G-Buffer instead of running composite shader
    DebugDrawGBufferColor();
    return;
    
    // Use composite shader
    BeginShaderMode(compositeShader);
    
    // ... rest of composite function (now unreachable due to return above)
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
    
    // Recreate G-Buffer with new size
    // Delete old resources first
    if (colorTexture != 0) glDeleteTextures(1, &colorTexture);
    if (normalTexture != 0) glDeleteTextures(1, &normalTexture);
    if (depthTexture != 0) glDeleteTextures(1, &depthTexture);
    if (depthBuffer != 0) glDeleteRenderbuffers(1, &depthBuffer);
    
    colorTexture = normalTexture = depthTexture = depthBuffer = 0;
    
    // Recreate
    CreateGBuffer(width, height);
}

// === SHADOW MAPPING ===

bool DeferredRenderer::CreateShadowMap() {
    // Generate shadow map FBO
    glGenFramebuffers(1, &shadowMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    
    // Create depth texture for shadow map
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
    
    // Attach depth texture to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture, 0);
    
    // No color buffer for shadow map
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Shadow map framebuffer incomplete!\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    fprintf(stderr, "Shadow map created (%dx%d)\n", shadowMapSize, shadowMapSize);
    return true;
}

void DeferredRenderer::BeginShadowPass(const Vector3& lightDir, const Vector3& sceneCenter, float sceneRadius) {
    if (!initialized) return;
    
    // Calculate light view matrix
    Vector3 lightPos = Vector3Add(sceneCenter, Vector3Scale(Vector3Normalize(lightDir), -sceneRadius * 2.0f));
    Matrix lightView = MatrixLookAt(lightPos, sceneCenter, (Vector3){0.0f, 1.0f, 0.0f});
    
    // Calculate orthographic projection for directional light
    float orthoSize = sceneRadius * 1.5f;
    Matrix lightProjection = MatrixOrtho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, sceneRadius * 4.0f);
    
    // Combined light space matrix
    lightSpaceMatrix = MatrixMultiply(lightView, lightProjection);
    
    // Bind shadow map FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    
    // Clear depth
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Cull front faces to reduce shadow acne
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}

void DeferredRenderer::EndShadowPass() {
    // Restore culling
    glCullFace(GL_BACK);
    
    // Unbind framebuffer and restore viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
}

void DeferredRenderer::CompositeWithShadows(const Camera3D& camera, const Vector3& lightDir, 
                                           const Vector3& lightColor, const Vector3& ambientColor) {
    if (!initialized) return;
    
    // DEBUG: Just draw raw color G-Buffer instead of running composite shader
    DebugDrawGBufferColor();
    return;
    
    // Use composite shader
    BeginShaderMode(compositeShader);
    
    // Bind G-Buffer textures to texture units 0, 1, 2
    int texUnit0 = 0;
    int texUnit1 = 1;
    int texUnit2 = 2;
    int texUnit3 = 3;
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    SetShaderValue(compositeShader, uColorTexture, &texUnit0, SHADER_UNIFORM_INT);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    SetShaderValue(compositeShader, uNormalTexture, &texUnit1, SHADER_UNIFORM_INT);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    SetShaderValue(compositeShader, uDepthTexture, &texUnit2, SHADER_UNIFORM_INT);
    
    // Bind shadow map to texture unit 3
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    SetShaderValue(compositeShader, uShadowMap, &texUnit3, SHADER_UNIFORM_INT);
    
    // Set lighting uniforms
    SetShaderValue(compositeShader, uLightDir, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(compositeShader, uLightColor, &lightColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(compositeShader, uAmbientColor, &ambientColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(compositeShader, uViewPos, &camera.position, SHADER_UNIFORM_VEC3);
    
    // Set shadow bias
    float shadowBias = 0.005f;
    SetShaderValue(compositeShader, uShadowBias, &shadowBias, SHADER_UNIFORM_FLOAT);
    
    // Set light space matrix
    SetShaderValueMatrix(compositeShader, uLightSpaceMatrix, lightSpaceMatrix);
    
    // Calculate view/projection matrices
    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
    float aspect = (float)screenWidth / (float)screenHeight;
    Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.01f, 1000.0f);
    
    Matrix invView = MatrixInvert(view);
    Matrix invProjection = MatrixInvert(projection);
    
    SetShaderValueMatrix(compositeShader, uInvView, invView);
    SetShaderValueMatrix(compositeShader, uInvProjection, invProjection);
    
    // Screen size
    float screenSize[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(compositeShader, uScreenSize, screenSize, SHADER_UNIFORM_VEC2);
    
    // Draw full screen quad
    DrawFullscreenQuad();
    
    // Unbind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    EndShaderMode();
}
