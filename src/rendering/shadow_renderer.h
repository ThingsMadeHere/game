#pragma once
#include <glad.h>
#include "raylib.h"

class ShadowRenderer {
public:
    ShadowRenderer();
    ~ShadowRenderer();
    
    bool Init(int shadowMapSize = 2048);
    void Cleanup();
    
    // Shadow pass - render scene from light's perspective
    void BeginShadowPass(const Vector3& lightDir, const Vector3& sceneCenter, float sceneRadius);
    void EndShadowPass();
    
    // Getters for main pass
    GLuint GetShadowMapTexture() const { return shadowMapTexture; }
    Matrix GetLightSpaceMatrix() const { return lightSpaceMatrix; }
    Shader& GetShadowShader() { return shadowShader; }
    Shader& GetMainShader() { return mainShader; }
    
    // Main pass setup
    void BeginMainPass(const Camera3D& camera, const Vector3& lightDir);
    void EndMainPass();
    
private:
    bool CreateShadowMap(int size);
    bool LoadShaders();
    
    GLuint shadowMapFBO = 0;
    GLuint shadowMapTexture = 0;
    int shadowMapSize = 2048;
    
    Shader shadowShader = {0};  // Depth only
    Shader mainShader = {0};    // With shadow sampling
    
    Matrix lightSpaceMatrix = {0};
    Vector3 currentLightDir = {0};
    
    bool initialized = false;
};
