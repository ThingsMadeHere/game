#pragma once
#include "raylib.h"
#include <glad.h>
#include <array>
#include <cstdint>

// Forward declaration
class BlockTextureAtlas;

/**
 * DeferredRenderer implements a deferred shading pipeline with:
 * - G-Buffer for geometry pass (color, normal, depth)
 * - Shadow mapping for directional lights
 * - Fullscreen composite pass for lighting calculation
 * 
 * G-Buffer attachments:
 * - Attachment 0: Color (RGBA8) - albedo/diffuse color
 * - Attachment 1: Normal (RGB16F) - world space normals
 * - Attachment 2: Depth (R32F) - linear depth
 */
class DeferredRenderer {
public:
    DeferredRenderer();
    ~DeferredRenderer();

    // Initialize the G-Buffer with screen dimensions
    bool Init(int width, int height);
    
    // Cleanup all GPU resources
    void Cleanup();
    
    // === GEOMETRY PASS ===
    // Begin geometry pass - renders to G-Buffer
    void BeginGeometryPass(const Camera3D& camera);
    void EndGeometryPass();
    
    // === SHADOW PASS ===
    // Begin shadow pass - renders depth from light's perspective
    void BeginShadowPass(const Vector3& lightDir, const Vector3& sceneCenter, float sceneRadius);
    void EndShadowPass();
    
    // === COMPOSITE PASS ===
    // Begin composite pass - renders to screen
    void BeginCompositePass();
    void EndCompositePass();
    
    // Composite final image to screen
    void Composite(const Camera3D& camera, const Vector3& lightDir, 
                   const Vector3& lightColor, const Vector3& ambientColor);
    
    // Composite with shadow map
    void CompositeWithShadows(const Camera3D& camera, const Vector3& lightDir, 
                              const Vector3& lightColor, const Vector3& ambientColor);
    
    // === RESIZE & STATE ===
    // Resize G-Buffer (call when window resizes)
    void Resize(int width, int height);
    
    // Check if initialized
    bool IsInitialized() const { return initialized; }
    
    // === DEBUG & UTILS ===
    // Debug: Draw raw G-Buffer color directly to screen
    void DebugDrawGBufferColor();
    
    // Full screen quad for composite pass
    void DrawFullscreenQuad();
    
    // Getters for G-Buffer textures (for debug visualization)
    GLuint GetColorTexture() const { return colorTexture; }
    GLuint GetNormalTexture() const { return normalTexture; }
    GLuint GetDepthTexture() const { return depthTexture; }
    GLuint GetShadowMapTexture() const { return shadowMapTexture; }
    
    // Get shaders for external use
    Shader& GetShadowShader() { return shadowShader; }
    Shader& GetGeometryShader() { return geometryShader; }
    
    // Get light space matrix for shadow mapping
    Matrix GetLightSpaceMatrix() const { return lightSpaceMatrix; }

private:
    // === CONSTANTS ===
    static constexpr int COLOR_ATTACHMENT_COUNT = 3;
    static constexpr int DEFAULT_SHADOW_MAP_SIZE = 2048;
    static constexpr float DEFAULT_SHADOW_BIAS = 0.005f;
    static constexpr float ORTHO_SCALE_FACTOR = 1.5f;
    static constexpr float LIGHT_DISTANCE_FACTOR = 2.0f;
    
    // === FRAMEBUFFER OBJECTS ===
    GLuint gBufferFBO = 0;
    GLuint shadowMapFBO = 0;
    
    // === G-BUFFER TEXTURES ===
    GLuint colorTexture = 0;    // RGBA8 - albedo/diffuse color
    GLuint normalTexture = 0;   // RGB16F - world space normals
    GLuint depthTexture = 0;    // R32F - linear depth
    GLuint depthRenderbuffer = 0; // Depth/stencil for depth testing
    
    // === SHADOW MAP ===
    GLuint shadowMapTexture = 0;
    int shadowMapSize = DEFAULT_SHADOW_MAP_SIZE;
    Matrix lightSpaceMatrix = {};
    
    // === FULLSCREEN QUAD ===
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;
    
    // === SHADERS ===
    Shader geometryShader = {};
    Shader compositeShader = {};
    Shader shadowShader = {};
    
    // === COMPOSITE SHADER UNIFORM LOCATIONS ===
    struct CompositeUniforms {
        int colorTexture = -1;
        int normalTexture = -1;
        int depthTexture = -1;
        int shadowMap = -1;
        int lightDir = -1;
        int lightColor = -1;
        int ambientColor = -1;
        int viewPos = -1;
        int invProjection = -1;
        int invView = -1;
        int screenSize = -1;
        int lightSpaceMatrix = -1;
        int shadowBias = -1;
    } compositeUniforms;
    
    // === SCREEN DIMENSIONS ===
    int screenWidth = 0;
    int screenHeight = 0;
    
    // === STATE ===
    bool initialized = false;
    
    // === PRIVATE METHODS ===
    void CreateFullscreenQuad();
    bool CreateGBuffer(int width, int height);
    bool CreateShadowMap();
    void DeleteGBufferResources();
    void DeleteShadowMapResources();
    void SetupCompositeShaderTextures(bool useShadows);
    void SetupCompositeShaderLighting(const Camera3D& camera, const Vector3& lightDir,
                                      const Vector3& lightColor, const Vector3& ambientColor);
};
