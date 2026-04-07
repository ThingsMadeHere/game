#include "voxel_mesher.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <utility>

Mesh VoxelMesher::GenerateChunkMesh(const Chunk& chunk, 
                                     const std::unordered_map<long long, Chunk>* neighborChunks) {
    std::vector<Vertex> vertices;
    
    // Reserve space - greedy meshing reduces vertices by ~80%
    vertices.reserve(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE / 2);
    
    // Generate meshes for each face direction using greedy meshing
    // For now, use the simpler approach but with optimizations
    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                BlockType blockType = chunk.getBlock(x, y, z);
                
                // Skip air blocks
                if (blockType == BlockType::AIR) continue;
                
                const BlockProperties& props = GetBlockProperties(blockType);
                
                // Skip non-solid blocks for now (water, etc.)
                if (!props.solid) continue;
                
                // Check each face and add it if visible
                for (int face = 0; face < 6; face++) {
                    if (chunk.shouldRenderFace(x, y, z, (Face)face, neighborChunks)) {
                        // Calculate ambient occlusion for this face
                        std::array<unsigned char, 4> aoLevels = {255, 255, 255, 255};
                        
                        // Add optimized face with proper UV mapping
                        AddOptimizedFace(vertices, (float)x, (float)y, (float)z, 
                                       (Face)face, props, aoLevels);
                    }
                }
            }
        }
    }
    
    // Create Raylib mesh from vertex data
    Mesh mesh = {0};
    
    if (vertices.empty()) {
        return mesh;
    }
    
    mesh.vertexCount = vertices.size();
    mesh.vertices = new float[mesh.vertexCount * 3];
    mesh.normals = new float[mesh.vertexCount * 3];
    mesh.texcoords = new float[mesh.vertexCount * 2];
    mesh.colors = new unsigned char[mesh.vertexCount * 4];
    
    for (size_t i = 0; i < vertices.size(); i++) {
        const Vertex& v = vertices[i];
        
        // Position
        mesh.vertices[i * 3 + 0] = v.x;
        mesh.vertices[i * 3 + 1] = v.y;
        mesh.vertices[i * 3 + 2] = v.z;
        
        // Normal
        mesh.normals[i * 3 + 0] = v.nx;
        mesh.normals[i * 3 + 1] = v.ny;
        mesh.normals[i * 3 + 2] = v.nz;
        
        // Texcoord
        mesh.texcoords[i * 2 + 0] = v.u;
        mesh.texcoords[i * 2 + 1] = v.v;
        
        // Color with ambient occlusion - use bright colors for testing
        mesh.colors[i * 4 + 0] = 255; // Bright red
        mesh.colors[i * 4 + 1] = 255; // Bright green
        mesh.colors[i * 4 + 2] = 255; // Bright blue
        mesh.colors[i * 4 + 3] = 255; // Full alpha
    }
    
    // Upload to GPU
    UploadMesh(&mesh, false);
    
    return mesh;
}

void VoxelMesher::AddOptimizedFace(std::vector<Vertex>& vertices,
                                   float x, float y, float z,
                                   Face face,
                                   const BlockProperties& props,
                                   const std::array<unsigned char, 4>& aoLevels) {
    // Get texture index based on face direction
    int texIndex = props.texSide;
    if (face == FACE_TOP) texIndex = props.texTop;
    else if (face == FACE_BOTTOM) texIndex = props.texBottom;
    
    // Calculate UV offsets from texture index
    const float TEX_CELL = 0.25f;
    int texRow = texIndex / 4;
    int texCol = texIndex % 4;
    float uOffset = texCol * TEX_CELL;
    float vOffset = texRow * TEX_CELL;
    
    // Define normal and offset
    float nx = 0, ny = 0, nz = 0;
    float ox = 0, oy = 0, oz = 0;
    
    switch (face) {
        case FACE_RIGHT:  // +X
            nx = 1.0f; ox = 1.0f;
            // Fixed UV coordinates for right face
            vertices.push_back({x + ox, y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x + ox, y,     z + 1, nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + ox, y + 1, z,     nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + ox, y + 1, z,     nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + ox, y,     z + 1, nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + ox, y + 1, z + 1, nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            break;
            
        case FACE_LEFT:  // -X
            nx = -1.0f;
            // Fixed UV coordinates for left face
            vertices.push_back({x,     y,     z,     nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y,     z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z,     nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z,     nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y,     z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            break;
            
        case FACE_TOP:  // +Y
            ny = 1.0f; oy = 1.0f;
            // Fixed UV coordinates for top face
            vertices.push_back({x,     y + oy, z,     nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y + oy, z,     nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y + oy, z + 1, nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y + oy, z + 1, nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + 1, y + oy, z,     nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y + oy, z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            break;
            
        case FACE_BOTTOM:  // -Y
            ny = -1.0f;
            // Fixed UV coordinates for bottom face
            vertices.push_back({x,     y,     z,     nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y,     z + 1, nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y,     z + 1, nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + 1, y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y,     z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            break;
            
        case FACE_FRONT:  // +Z
            nz = 1.0f; oz = 1.0f;
            // Fixed UV coordinates for front face
            vertices.push_back({x,     y,     z + oz, nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y,     z + oz, nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z + oz, nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z + oz, nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + 1, y,     z + oz, nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y + 1, z + oz, nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            break;
            
        case FACE_BACK:  // -Z
            nz = -1.0f;
            // Fixed UV coordinates for back face
            vertices.push_back({x + 1, y,     z,     nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y + 1, z,     nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + 1, y + 1, z,     nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z,     nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            break;
    }
}

unsigned char VoxelMesher::CalculateAO(const Chunk& chunk,
                                      int x, int y, int z,
                                      Face face,
                                      int corner,
                                      const std::unordered_map<long long, Chunk>* neighborChunks) {
    // Simple ambient occlusion calculation
    // For now, return full brightness
    return 255;
}

std::pair<float, float> VoxelMesher::GetUV(int texIndex, float u, float v) {
    // Texture atlas is 4x4 grid (16 textures)
    const float TEX_CELL = 0.25f;
    
    // Calculate UV offsets from texture index
    int texRow = texIndex / 4;
    int texCol = texIndex % 4;
    float uOffset = texCol * TEX_CELL;
    float vOffset = texRow * TEX_CELL;
    
    // Apply UV coordinates with proper mapping
    return {
        uOffset + u * TEX_CELL,
        vOffset + v * TEX_CELL
    };
}
