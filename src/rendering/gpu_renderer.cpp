#include "gpu_renderer.h"
#include "../terrain/terrain.h"
#include <algorithm>
#include <cstdio>

void GPUChunkRenderer::Init() {
    // Load the voxel shader
    voxelShader = LoadShader("shaders/voxel.vs", "shaders/voxel.fs");
    
    if (voxelShader.id == 0) {
        fprintf(stderr,"Failed to load voxel shader!\n");
        return;
    }
    
    // Create a simple cube mesh for rendering chunks
    Mesh cubeMesh = GenMeshCube(CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE);
    chunkModel = LoadModelFromMesh(cubeMesh);
    
    // Assign shader to model
    chunkModel.materials[0].shader = voxelShader;
    
    fprintf(stderr,"GPU Chunk Renderer initialized\n");
}

void GPUChunkRenderer::RenderChunk(const Chunk& chunk, const Camera3D& camera) {
    // Only render chunks with valid generated meshes - no fallback
    if (chunk.meshGenerated && chunk.mesh.vertexCount > 0 && chunk.mesh.vertices != nullptr) {
        fprintf(stderr,"Rendering chunk (%d,%d,%d) with %d vertices\n", 
               chunk.cx, chunk.cy, chunk.cz, chunk.mesh.vertexCount);
        
        // Skip chunks with too many vertices (potential crash)
        if (chunk.mesh.vertexCount > 5000) {
            fprintf(stderr,"Chunk (%d,%d,%d) has too many vertices (%d) - skipping for stability\n", 
                   chunk.cx, chunk.cy, chunk.cz, chunk.mesh.vertexCount);
            return;
        }
        
        // Create a temporary model from the chunk mesh for efficient rendering
        Model chunkModel = LoadModelFromMesh(chunk.mesh);
        
        // Additional safety check - ensure model is valid
        if (chunkModel.meshes == nullptr || chunkModel.meshCount == 0) {
            fprintf(stderr,"Invalid model for chunk (%d,%d,%d) - skipping\n", 
                   chunk.cx, chunk.cy, chunk.cz);
            return;
        }
        
        // Draw the entire mesh in one call - this is the key optimization!
        // Draw at chunk position directly
        Vector3 chunkPosition = {
            chunk.cx * CHUNK_SIZE * VOXEL_SIZE,
            chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE, 
            chunk.cz * CHUNK_SIZE * VOXEL_SIZE
        };
        DrawModel(chunkModel, chunkPosition, 1.0f, WHITE);
        
        // Unload the temporary model (but not the mesh - it's owned by the chunk)
        chunkModel.materials[0].shader = (Shader){0}; // Prevent shader unloading
        UnloadModel(chunkModel);
        
    } else {
        // No mesh available - skip rendering (chunks should be generated before rendering)
        fprintf(stderr,"No mesh for chunk (%d,%d,%d) - skipping render\n", 
               chunk.cx, chunk.cy, chunk.cz);
    }
}

void GPUChunkRenderer::Cleanup() {
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
