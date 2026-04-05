#include "voxel_mesher.h"
#include <cstdio>
#include <cstring>

Mesh VoxelMesher::GenerateChunkMesh(const Chunk& chunk, 
                                     const std::unordered_map<long long, Chunk>* neighborChunks) {
    std::vector<Vertex> vertices;
    
    // Reserve space to avoid reallocations (estimate ~30% of blocks will have visible faces)
    vertices.reserve(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE * 2);
    
    // Iterate through all voxels in the chunk
    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                BlockType blockType = chunk.getBlock(x, y, z);
                
                // Skip air blocks
                if (blockType == BlockType::AIR) continue;
                
                const BlockProperties& props = GetBlockProperties(blockType);
                
                // Skip non-solid blocks for now (water, etc.)
                // They need special rendering
                
                // Check each face and add it if visible
                for (int face = 0; face < 6; face++) {
                    if (chunk.shouldRenderFace(x, y, z, (Face)face, neighborChunks)) {
                        AddFace(vertices, (float)x, (float)y, (float)z, 
                               (Face)face, props, neighborChunks, 
                               chunk.cx, chunk.cy, chunk.cz);
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
        
        // Color
        mesh.colors[i * 4 + 0] = v.r;
        mesh.colors[i * 4 + 1] = v.g;
        mesh.colors[i * 4 + 2] = v.b;
        mesh.colors[i * 4 + 3] = v.a;
    }
    
    // Upload to GPU
    UploadMesh(&mesh, false);
    
    return mesh;
}

void VoxelMesher::AddFace(std::vector<Vertex>& vertices,
                          float x, float y, float z,
                          Face face,
                          const BlockProperties& props,
                          const std::unordered_map<long long, Chunk>* neighborChunks,
                          int cx, int cy, int cz) {
    // Texture atlas is assumed to be 4x4 grid (16 textures)
    // Each cell is 0.25 UV units
    const float TEX_CELL = 0.25f;
    
    // Get texture index based on face direction
    int texIndex = props.texSide;
    if (face == FACE_TOP) texIndex = props.texTop;
    else if (face == FACE_BOTTOM) texIndex = props.texBottom;
    
    // Calculate UV offsets from texture index
    int texRow = texIndex / 4;
    int texCol = texIndex % 4;
    float uOffset = texCol * TEX_CELL;
    float vOffset = texRow * TEX_CELL;
    
    // Define quad vertices based on face direction
    // Each face is 2 triangles = 6 vertices
    
    float nx = 0, ny = 0, nz = 0;  // Normal
    float ox = 0, oy = 0, oz = 0;  // Offset for position
    
    switch (face) {
        case FACE_RIGHT:  // +X
            nx = 1.0f; ox = 1.0f;
            vertices.push_back({x + ox, y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x + ox, y,     z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + ox, y + 1, z,     nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + ox, y + 1, z,     nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + ox, y,     z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + ox, y + 1, z + 1, nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            break;
            
        case FACE_LEFT:  // -X
            nx = -1.0f;
            vertices.push_back({x,     y,     z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z + 1, nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z + 1, nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z,     nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            break;
            
        case FACE_TOP:  // +Y
            ny = 1.0f; oy = 1.0f;
            vertices.push_back({x,     y + oy, z + 1, nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + 1, y + oy, z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y + oy, z,     nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y + oy, z,     nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y + oy, z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + 1, y + oy, z,     nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            break;
            
        case FACE_BOTTOM:  // -Y
            ny = -1.0f;
            vertices.push_back({x,     y,     z,     nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + 1, y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y,     z + 1, nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y,     z + 1, nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + 1, y,     z + 1, nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            break;
            
        case FACE_FRONT:  // +Z
            nz = 1.0f; oz = 1.0f;
            vertices.push_back({x + 1, y,     z + oz, nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y,     z + oz, nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + 1, y + 1, z + oz, nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y + 1, z + oz, nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y,     z + oz, nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z + oz, nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            break;
            
        case FACE_BACK:  // -Z
            nz = -1.0f;
            vertices.push_back({x,     y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z,     nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x,     y + 1, z,     nx, ny, nz, uOffset,            vOffset,         255, 255, 255, 255});
            vertices.push_back({x + 1, y,     z,     nx, ny, nz, uOffset + TEX_CELL, vOffset + TEX_CELL, 255, 255, 255, 255});
            vertices.push_back({x + 1, y + 1, z,     nx, ny, nz, uOffset,            vOffset + TEX_CELL, 255, 255, 255, 255});
            break;
    }
}
