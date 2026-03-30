#include "gpu_renderer.h"
#include "terrain.h"
#include <cstdio>
#include <algorithm>

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
        printf("Rendering chunk (%d,%d,%d) with %d vertices\n", 
               chunk.cx, chunk.cy, chunk.cz, chunk.mesh.vertexCount);
        
        // Draw the mesh as triangles with proper shading and lighting
        // Overlap by exactly 1 voxel (0.5 units) to eliminate 1-voxel gaps
        const float VOXEL_OVERLAP = VOXEL_SIZE; // 0.5f = exactly 1 voxel overlap
        for (int i = 0; i < chunk.mesh.vertexCount; i += 3) {
            Vector3 v1 = {
                chunk.mesh.vertices[i*3] + chunk.cx * CHUNK_SIZE * VOXEL_SIZE - VOXEL_OVERLAP,
                chunk.mesh.vertices[i*3+1] + chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE - VOXEL_OVERLAP,
                chunk.mesh.vertices[i*3+2] + chunk.cz * CHUNK_SIZE * VOXEL_SIZE - VOXEL_OVERLAP
            };
            Vector3 v2 = {
                chunk.mesh.vertices[(i+1)*3] + chunk.cx * CHUNK_SIZE * VOXEL_SIZE - VOXEL_OVERLAP,
                chunk.mesh.vertices[(i+1)*3+1] + chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE - VOXEL_OVERLAP,
                chunk.mesh.vertices[(i+1)*3+2] + chunk.cz * CHUNK_SIZE * VOXEL_SIZE - VOXEL_OVERLAP
            };
            Vector3 v3 = {
                chunk.mesh.vertices[(i+2)*3] + chunk.cx * CHUNK_SIZE * VOXEL_SIZE - VOXEL_OVERLAP,
                chunk.mesh.vertices[(i+2)*3+1] + chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE - VOXEL_OVERLAP,
                chunk.mesh.vertices[(i+2)*3+2] + chunk.cz * CHUNK_SIZE * VOXEL_SIZE - VOXEL_OVERLAP
            };
            
            // Get face normal from mesh data
            Vector3 normal = {
                chunk.mesh.normals[i*3],
                chunk.mesh.normals[i*3+1], 
                chunk.mesh.normals[i*3+2]
            };
            
            // Calculate lighting based on face normal and sun direction
            Vector3 sunDirection = {0.7f, 0.7f, 0.2f}; // Sun from top-right
            float brightness = 0.3f; // Ambient light
            
            // Calculate dot product for diffuse lighting
            float dotProduct = normal.x * sunDirection.x + normal.y * sunDirection.y + normal.z * sunDirection.z;
            brightness += std::max(0.0f, dotProduct) * 0.7f; // Diffuse lighting
            
            // Clamp brightness
            if (brightness < 0.2f) brightness = 0.2f;
            if (brightness > 1.0f) brightness = 1.0f;
            
            // Apply lighting to base color (green terrain)
            Color baseColor = {34, 139, 34, 255}; // Forest green
            Color litColor = {
                (unsigned char)(baseColor.r * brightness),
                (unsigned char)(baseColor.g * brightness),
                (unsigned char)(baseColor.b * brightness),
                baseColor.a
            };
            
            DrawTriangle3D(v1, v2, v3, litColor);
        }
    } else {
        // Fallback: render chunk boundaries for debugging
        const float VOXEL_OVERLAP = VOXEL_SIZE; // 0.5f = exactly 1 voxel overlap
        Vector3 chunkPos = {
            (float)chunk.cx * CHUNK_SIZE * VOXEL_SIZE - VOXEL_OVERLAP,
            (float)chunk.cy * CHUNK_HEIGHT * VOXEL_SIZE - VOXEL_OVERLAP,
            (float)chunk.cz * CHUNK_SIZE * VOXEL_SIZE - VOXEL_OVERLAP
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
