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
    // Use generated mesh from marching cubes if available
    if (chunk.meshGenerated && chunk.mesh.vertexCount > 0) {
        // Set shader uniforms
        Shader shader = voxelShader;
        
        // Set chunk position
        Vector3 chunkPos = {
            (float)chunk.cx * CHUNK_SIZE * VOXEL_SIZE,
            (float)chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE,
            (float)chunk.cz * CHUNK_SIZE * VOXEL_SIZE
        };
        SetShaderValue(shader, GetShaderLocation(shader, "chunkPosition"), &chunkPos, SHADER_UNIFORM_VEC3);
        
        // Draw the mesh as triangles
        for (int i = 0; i < chunk.mesh.vertexCount; i += 3) {
            Vector3 v1 = {
                chunk.mesh.vertices[i*3] + chunk.cx * CHUNK_SIZE * VOXEL_SIZE,
                chunk.mesh.vertices[i*3+1] + chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE,
                chunk.mesh.vertices[i*3+2] + chunk.cz * CHUNK_SIZE * VOXEL_SIZE
            };
            Vector3 v2 = {
                chunk.mesh.vertices[(i+1)*3] + chunk.cx * CHUNK_SIZE * VOXEL_SIZE,
                chunk.mesh.vertices[(i+1)*3+1] + chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE,
                chunk.mesh.vertices[(i+1)*3+2] + chunk.cz * CHUNK_SIZE * VOXEL_SIZE
            };
            Vector3 v3 = {
                chunk.mesh.vertices[(i+2)*3] + chunk.cx * CHUNK_SIZE * VOXEL_SIZE,
                chunk.mesh.vertices[(i+2)*3+1] + chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE,
                chunk.mesh.vertices[(i+2)*3+2] + chunk.cz * CHUNK_SIZE * VOXEL_SIZE
            };
            
            // Draw triangle
            DrawTriangle3D(v1, v2, v3, LIGHTGRAY);
        }
    } else {
        // Fallback: render chunk boundaries for debugging
        Vector3 chunkPos = {
            (float)chunk.cx * CHUNK_SIZE * VOXEL_SIZE,
            (float)chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE,
            (float)chunk.cz * CHUNK_SIZE * VOXEL_SIZE
        };
        Vector3 chunkSize = {
            (float)CHUNK_SIZE * VOXEL_SIZE, 
            (float)CHUNK_HEIGHT * VOXEL_SIZE, 
            (float)CHUNK_SIZE * VOXEL_SIZE
        };
        DrawCubeWires(chunkPos, chunkSize.x, chunkSize.y, chunkSize.z, RED);
    }
}

void GPUChunkRenderer::Cleanup() {
    if (voxelShader.id != 0) {
        UnloadShader(voxelShader);
    }
    if (chunkModel.meshCount > 0) {
        UnloadModel(chunkModel);
    }
}
