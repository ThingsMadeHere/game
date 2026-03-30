#include "gpu_renderer.h"
#include <cstdio>

void GPUChunkRenderer::Init() {
    // Load the voxel shader
    voxelShader = LoadShader("shaders/voxel.vs", "shaders/voxel.fs");
    
    if (voxelShader.id == 0) {
        printf("Failed to load voxel shader!\n");
        return;
    }
    
    // Create a simple cube mesh for rendering chunks
    Mesh cubeMesh = GenMeshCube(CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE);
    chunkModel = LoadModelFromMesh(cubeMesh);
    
    // Assign shader to model
    chunkModel.materials[0].shader = voxelShader;
    
    printf("GPU Chunk Renderer initialized\n");
}

void GPUChunkRenderer::RenderChunk(const Chunk& chunk, const Camera3D& camera) {
    if (voxelShader.id == 0 || chunk.densityTexture.id == 0) {
        return;
    }
    
    // Set shader uniforms
    Shader shader = voxelShader;
    
    // Set density texture
    SetShaderValueTexture(shader, GetShaderLocation(shader, "densityTexture"), chunk.densityTexture);
    
    // Set chunk position
    Vector3 chunkPos = {
        (float)chunk.cx * CHUNK_SIZE,
        (float)chunk.cy * CHUNK_HEIGHT,
        (float)chunk.cz * CHUNK_SIZE
    };
    SetShaderValue(shader, GetShaderLocation(shader, "chunkPosition"), &chunkPos, SHADER_UNIFORM_VEC3);
    
    // Set voxel size
    SetShaderValue(shader, GetShaderLocation(shader, "voxelSize"), &VOXEL_SIZE, SHADER_UNIFORM_FLOAT);
    
    // Draw the chunk model with the shader
    DrawModel(chunkModel, chunkPos, 1.0f, WHITE);
}

void GPUChunkRenderer::Cleanup() {
    if (voxelShader.id != 0) {
        UnloadShader(voxelShader);
    }
    if (chunkModel.meshCount > 0) {
        UnloadModel(chunkModel);
    }
}
