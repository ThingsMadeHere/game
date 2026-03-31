#pragma once
#include "raylib.h"
#include <glad.h>

// Forward declaration
class BlockTextureAtlas;

// G-Buffer attachments:
// 0: Color (RGB) 
// 1: Normal (RGB)
// 2: Depth (R - linear depth)

class DeferredRenderer {
public:
    DeferredRenderer();
    ~DeferredRenderer();

    // Initialize the G-Buffer with screen dimensions
    bool Init(int width, int height);
    
    // Cleanup all GPU resources
    void Cleanup();
    
    // Begin geometry pass - renders to G-Buffer
    void BeginGeometryPass(const Camera3D& camera);
    
    // End geometry pass
    void EndGeometryPass();
    
    // Begin composite pass - renders to screen
    void BeginCompositePass();
    
    // End composite pass
    void EndCompositePass();
    
    // Composite final image using the composite shader (without shadows)
    void Composite(const Camera3D& camera, const Vector3& lightDir, const Vector3& lightColor, const Vector3& ambientColor);
    
    // Composite with shadow map
    void CompositeWithShadows(const Camera3D& camera, const Vector3& lightDir, const Vector3& lightColor, const Vector3& ambientColor);
    
    // Begin shadow pass - renders depth from light's perspective
    void BeginShadowPass(const Vector3& lightDir, const Vector3& sceneCenter, float sceneRadius);
    void EndShadowPass();
    
    // Get shadow map texture for debugging
    GLuint GetShadowMapTexture() const { return shadowMapTexture; }
    
    // Get shadow shader for shadow pass rendering
    Shader& GetShadowShader() { return shadowShader; }
    
    // Get geometry shader for G-Buffer pass
    Shader& GetGeometryShader() { return geometryShader; }
    
    // Get light space matrix for shadow mapping
    Matrix GetLightSpaceMatrix() const { return lightSpaceMatrix; }
    
    // Resize G-Buffer (call when window resizes)
    void Resize(int width, int height);
    
    // Getters for G-Buffer textures (for debug visualization)
    GLuint GetColorTexture() const { return colorTexture; }
    GLuint GetNormalTexture() const { return normalTexture; }
    GLuint GetDepthTexture() const { return depthTexture; }
    
    // Debug: Draw raw G-Buffer color directly to screen
    void DebugDrawGBufferColor();
    
    // Full screen quad for composite pass
    void DrawFullscreenQuad();
    
    // Check if initialized
    bool IsInitialized() const { return initialized; }

private:
    // Framebuffer Object
    GLuint fbo;
    
    // G-Buffer textures
    GLuint colorTexture;    // RGB8 - albedo/diffuse color
    GLuint normalTexture;   // RGB16F - world space normals
    GLuint depthTexture;    // R32F - linear depth
    
    // Depth buffer (for depth testing during geometry pass)
    GLuint depthBuffer;
    
    // Screen dimensions
    int screenWidth;
    int screenHeight;
    
    // Full screen quad VAO/VBO
    GLuint quadVAO;
    GLuint quadVBO;
    
    // Shaders
    Shader geometryShader;
    Shader compositeShader;
    
    // Uniform locations for composite shader
    int uColorTexture;
    int uNormalTexture;
    int uDepthTexture;
    int uLightDir;
    int uLightColor;
    int uAmbientColor;
    int uViewPos;
    int uInvProjection;
    int uInvView;
    int uScreenSize;
    
    // Shadow map uniforms
    int uShadowMap;
    int uLightSpaceMatrix;
    int uShadowBias;
    
    // Shadow mapping
    GLuint shadowMapFBO;
    GLuint shadowMapTexture;
    Shader shadowShader;
    Matrix lightSpaceMatrix;
    int shadowMapSize;
    
    bool initialized;
    
    // Create full screen quad
    void CreateFullscreenQuad();
    
    // Setup G-Buffer textures
    bool CreateGBuffer(int width, int height);
    
    // Setup shadow map
    bool CreateShadowMap();
};
