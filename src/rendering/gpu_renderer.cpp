#include "gpu_renderer.h"
#include "../terrain/terrain.h"
#include "texture_atlas.h"
#include <glad.h>
#include <raymath.h>
#include <algorithm>
#include <cstdio>

void GPUChunkRenderer::Init() {
    // Load the voxel shader
    voxelShader = LoadShader("/home/carter/Desktop/Farout/shaders/voxel.vs", "/home/carter/Desktop/Farout/shaders/voxel.fs");
    
    fprintf(stderr,"Loaded shader ID: %d\n", voxelShader.id);
    
    if (voxelShader.id == 0) {
        fprintf(stderr,"Failed to load voxel shader!\n");
        return;
    }
    
    // Uniform locations
    uDensityTexture = GetShaderLocation(voxelShader, "densityTexture");
    uChunkPosition = GetShaderLocation(voxelShader, "chunkPosition");
    uVoxelSize = GetShaderLocation(voxelShader, "voxelSize");
    
    // Get lighting uniform locations
    uGrassTexture = GetShaderLocation(voxelShader, "grassTexture");
    uLightDir = GetShaderLocation(voxelShader, "lightDir");
    uLightColor = GetShaderLocation(voxelShader, "lightColor");
    uAmbientColor = GetShaderLocation(voxelShader, "ambientColor");
    uViewPos = GetShaderLocation(voxelShader, "viewPos");
    
    // Create a default 3D density texture using OpenGL directly
    int texSize = 16;
    unsigned char* data = new unsigned char[texSize * texSize * texSize];
    for (int i = 0; i < texSize * texSize * texSize; i++) {
        data[i] = 255; // White = solid
    }
    
    glGenTextures(1, &densityTextureID);
    glBindTexture(GL_TEXTURE_3D, densityTextureID);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, texSize, texSize, texSize, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_3D, 0);
    
    delete[] data;
    
    // Create texture atlas with all block types
    textureAtlas.Init();
    
    // Create a simple cube mesh for rendering chunks
    Mesh cubeMesh = GenMeshCube(CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE);
    chunkModel = LoadModelFromMesh(cubeMesh);
    
    // Assign shader to model
    chunkModel.materials[0].shader = voxelShader;
    
    // Set texture atlas on the material
    chunkModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = textureAtlas.GetTexture();
    
    fprintf(stderr,"GPU Chunk Renderer initialized\n");
}

void GPUChunkRenderer::RenderChunk(const Chunk& chunk, const Camera3D& camera) {
    // Only render chunks with valid generated meshes - no fallback
    if (chunk.meshGenerated && chunk.mesh.vertexCount > 0 && chunk.mesh.vertices != nullptr) {
        fprintf(stderr,"Rendering chunk (%d,%d,%d) with %d vertices\n", 
               chunk.cx, chunk.cy, chunk.cz, chunk.mesh.vertexCount);
        
        // Skip chunks with too many vertices (potential crash)
        if (chunk.mesh.vertexCount > 50000) {
            fprintf(stderr,"Chunk (%d,%d,%d) has too many vertices (%d) - skipping for stability\n", 
                   chunk.cx, chunk.cy, chunk.cz, chunk.mesh.vertexCount);
            return;
        }
        
        // Set shader uniforms
        Vector3 chunkPos = {
            chunk.cx * CHUNK_SIZE * VOXEL_SIZE,
            chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE, 
            chunk.cz * CHUNK_SIZE * VOXEL_SIZE
        };
        SetShaderValue(voxelShader, uChunkPosition, &chunkPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(voxelShader, uVoxelSize, &VOXEL_SIZE, SHADER_UNIFORM_FLOAT);
        
        // Set lighting uniforms
        Vector3 lightDir = {0.5f, -1.0f, 0.3f}; // Light coming from above-right
        SetShaderValue(voxelShader, uLightDir, &lightDir, SHADER_UNIFORM_VEC3);
        
        Vector3 lightColor = {1.0f, 1.0f, 0.9f}; // Warm white light
        SetShaderValue(voxelShader, uLightColor, &lightColor, SHADER_UNIFORM_VEC3);
        
        Vector3 ambientColor = {0.3f, 0.35f, 0.4f}; // Soft blue ambient
        SetShaderValue(voxelShader, uAmbientColor, &ambientColor, SHADER_UNIFORM_VEC3);
        
        SetShaderValue(voxelShader, uViewPos, &camera.position, SHADER_UNIFORM_VEC3);
        
        // Bind texture atlas to texture unit 1
        SetShaderValueTexture(voxelShader, uGrassTexture, textureAtlas.GetTexture());
        
        // Draw mesh using material with grass texture and shader
        Matrix transform = MatrixMultiply(
            MatrixScale(1.0f, 1.0f, 1.0f),
            MatrixTranslate(chunkPos.x, chunkPos.y, chunkPos.z)
        );
        
        // Use texture atlas material
        Material terrainMat = LoadMaterialDefault();
        terrainMat.maps[MATERIAL_MAP_DIFFUSE].texture = textureAtlas.GetTexture();
        
        // Enable lighting
        terrainMat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        terrainMat.maps[MATERIAL_MAP_DIFFUSE].value = 1; // Enable diffuse lighting
        
        DrawMesh(chunk.mesh, terrainMat, transform);
        
        // DEBUG: Draw a wireframe cube at chunk position to verify location
        DrawCubeWires(chunkPos, CHUNK_SIZE * VOXEL_SIZE, CHUNK_HEIGHT * VOXEL_SIZE, 
                      CHUNK_SIZE * VOXEL_SIZE, YELLOW);
        
    } else {
        // No mesh available - skip rendering (chunks should be generated before rendering)
        fprintf(stderr,"No mesh for chunk (%d,%d,%d) - skipping render\n", 
               chunk.cx, chunk.cy, chunk.cz);
    }
}

void GPUChunkRenderer::RenderChunkShadow(const Chunk& chunk, const Matrix& lightSpaceMatrix, Shader& shadowShader) {
    // Only render chunks with valid generated meshes
    if (chunk.meshGenerated && chunk.mesh.vertexCount > 0 && chunk.mesh.vertices != nullptr) {
        // Skip chunks with too many vertices (potential crash)
        if (chunk.mesh.vertexCount > 50000) {
            return;
        }
        
        // Calculate chunk position
        Vector3 chunkPos = {
            chunk.cx * CHUNK_SIZE * VOXEL_SIZE,
            chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE, 
            chunk.cz * CHUNK_SIZE * VOXEL_SIZE
        };
        
        // Model matrix
        Matrix model = MatrixTranslate(chunkPos.x, chunkPos.y, chunkPos.z);
        
        // Set shadow shader uniforms
        int uLightSpaceMatrix = GetShaderLocation(shadowShader, "lightSpaceMatrix");
        int uModel = GetShaderLocation(shadowShader, "model");
        
        SetShaderValueMatrix(shadowShader, uLightSpaceMatrix, lightSpaceMatrix);
        SetShaderValueMatrix(shadowShader, uModel, model);
        
        // Draw mesh with shadow shader
        Material mat = LoadMaterialDefault();
        mat.shader = shadowShader;
        DrawMesh(chunk.mesh, mat, MatrixIdentity());
    }
}

void GPUChunkRenderer::Cleanup() {
    if (densityTextureID != 0) {
        glDeleteTextures(1, &densityTextureID);
        densityTextureID = 0;
    }
    
    // Cleanup texture atlas
    textureAtlas.Cleanup();
    
    if (voxelShader.id != 0) {
        UnloadShader(voxelShader);
        voxelShader = {0};
    }
    
    if (chunkModel.meshes != nullptr) {
        UnloadModel(chunkModel);
        chunkModel = {0};
    }
    
    printf("GPU Chunk Renderer cleaned up\n");
}

// Generate a 32x32 procedural grass texture
Texture2D GPUChunkRenderer::GenerateGrassTexture() {
    int size = 32;
    Color* pixels = new Color[size * size];
    
    // Base grass colors
    Color darkGreen = {34, 139, 34, 255};    // Forest green
    Color lightGreen = {50, 205, 50, 255};   // Lime green
    Color brown = {139, 69, 19, 255};        // Saddle brown for dirt patches
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Simple noise-like pattern
            float noise = (float)(rand() % 100) / 100.0f;
            
            // Create grass pattern with variation
            Color pixel;
            if (noise < 0.1f) {
                pixel = brown; // Occasional dirt patches
            } else if (noise < 0.6f) {
                pixel = darkGreen;
            } else {
                pixel = lightGreen;
            }
            
            // Add some vertical streaks for grass blades
            if ((x + y) % 3 == 0 && noise > 0.3f) {
                pixel.r = (unsigned char)fminf(255, pixel.r + 20);
                pixel.g = (unsigned char)fminf(255, pixel.g + 30);
            }
            
            pixels[y * size + x] = pixel;
        }
    }
    
    // Create image from pixels
    Image grassImage = {
        pixels,
        size,
        size,
        1,
        PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    
    // Load texture from image
    Texture2D texture = LoadTextureFromImage(grassImage);
    
    // Set texture to repeat
    SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
    
    // Unload image (data is copied to GPU)
    UnloadImage(grassImage);
    
    fprintf(stderr,"Generated grass texture: %dx%d\n", size, size);
    return texture;
}
