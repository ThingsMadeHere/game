#include "voxel_mesher.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <utility>

// ============================================================================
// VoxelMesher - Complete Rewrite with Greedy Meshing
// ============================================================================

Mesh VoxelMesher::GenerateChunkMesh(const Chunk& chunk, 
                                     const std::unordered_map<long long, Chunk>* neighborChunks) {
    std::vector<Vertex> vertices;
    
    // Reserve space - greedy meshing typically reduces vertices by 80-95%
    vertices.reserve(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE);
    
    // Generate meshes for all 6 face directions using greedy meshing
    // Axis 0 = Y faces (top/bottom), Axis 1 = X faces (right/left), Axis 2 = Z faces (front/back)
    for (int axis = 0; axis < 3; axis++) {
        // Positive direction
        GenerateGreedyMesh(chunk, vertices, axis, true, neighborChunks);
        // Negative direction
        GenerateGreedyMesh(chunk, vertices, axis, false, neighborChunks);
    }
    
    // Create Raylib mesh from vertex data
    Mesh mesh = {0};
    
    if (vertices.empty()) {
        return mesh;
    }
    
    mesh.vertexCount = static_cast<int>(vertices.size());
    mesh.vertices = new float[mesh.vertexCount * 3];
    mesh.normals = new float[mesh.vertexCount * 3];
    mesh.texcoords = new float[mesh.vertexCount * 2];
    mesh.colors = new unsigned char[mesh.vertexCount * 4];
    
    for (int i = 0; i < mesh.vertexCount; i++) {
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
        
        // Color with ambient occlusion
        mesh.colors[i * 4 + 0] = v.r;
        mesh.colors[i * 4 + 1] = v.g;
        mesh.colors[i * 4 + 2] = v.b;
        mesh.colors[i * 4 + 3] = v.a;
    }
    
    // Upload to GPU
    UploadMesh(&mesh, false);
    
    return mesh;
}

void VoxelMesher::BuildFaceMask(
    const Chunk& chunk,
    bool* mask,
    int axis,
    bool positiveDir,
    const std::unordered_map<long long, Chunk>* neighborChunks
) {
    memset(mask, 0, sizeof(bool) * CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE);
    
    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                BlockType blockType = chunk.getBlock(x, y, z);
                
                // Skip air and non-solid blocks
                if (blockType == BlockType::AIR) continue;
                
                const BlockProperties& props = GetBlockProperties(blockType);
                if (!props.solid) continue;
                
                // Determine neighbor coordinates based on axis and direction
                int nx = x, ny = y, nz = z;
                
                switch (axis) {
                    case 0: // Y axis (top/bottom)
                        ny = positiveDir ? y + 1 : y - 1;
                        break;
                    case 1: // X axis (right/left)
                        nx = positiveDir ? x + 1 : x - 1;
                        break;
                    case 2: // Z axis (front/back)
                        nz = positiveDir ? z + 1 : z - 1;
                        break;
                }
                
                // Check if face should be rendered
                bool renderFace = false;
                
                if (chunk.isValid(nx, ny, nz)) {
                    BlockType neighbor = chunk.getBlock(nx, ny, nz);
                    if (neighbor == BlockType::AIR) {
                        renderFace = true;
                    } else {
                        const BlockProperties& neighborProps = GetBlockProperties(neighbor);
                        // Render face between different transparency types
                        if (props.transparent != neighborProps.transparent) {
                            renderFace = true;
                        }
                    }
                } else {
                    // Neighbor is in another chunk
                    int ncx = chunk.cx, ncy = chunk.cy, ncz = chunk.cz;
                    int localX = nx, localY = ny, localZ = nz;
                    
                    // Wrap coordinates
                    if (localX < 0) { ncx--; localX += CHUNK_SIZE; }
                    else if (localX >= CHUNK_SIZE) { ncx++; localX -= CHUNK_SIZE; }
                    
                    if (localY < 0) { ncy--; localY += CHUNK_HEIGHT; }
                    else if (localY >= CHUNK_HEIGHT) { ncy++; localY -= CHUNK_HEIGHT; }
                    
                    if (localZ < 0) { ncz--; localZ += CHUNK_SIZE; }
                    else if (localZ >= CHUNK_SIZE) { ncz++; localZ -= CHUNK_SIZE; }
                    
                    long long key = makeChunkKey(ncx, ncy, ncz);
                    auto it = neighborChunks ? neighborChunks->find(key) : neighborChunks->end();
                    
                    if (it != neighborChunks->end()) {
                        BlockType neighbor = it->second.getBlock(localX, localY, localZ);
                        if (neighbor == BlockType::AIR) {
                            renderFace = true;
                        } else {
                            const BlockProperties& neighborProps = GetBlockProperties(neighbor);
                            if (props.transparent != neighborProps.transparent) {
                                renderFace = true;
                            }
                        }
                    } else {
                        // No neighbor chunk - assume air
                        renderFace = true;
                    }
                }
                
                if (renderFace) {
                    mask[chunk.getIndex(x, y, z)] = true;
                }
            }
        }
    }
}

void VoxelMesher::GenerateGreedyMesh(
    const Chunk& chunk,
    std::vector<Vertex>& vertices,
    int axis,
    bool positiveDir,
    const std::unordered_map<long long, Chunk>* neighborChunks
) {
    // Build face mask
    bool* mask = new bool[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE];
    BuildFaceMask(chunk, mask, axis, positiveDir, neighborChunks);
    
    // Track which voxels have been merged
    bool* merged = new bool[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE];
    memset(merged, 0, sizeof(bool) * CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE);
    
    // Determine iteration order based on axis
    // For Y faces (axis=0): iterate over XZ plane
    // For X faces (axis=1): iterate over YZ plane  
    // For Z faces (axis=2): iterate over XY plane
    
    int dim1, dim2, fixedDim;
    if (axis == 0) { dim1 = 0; dim2 = 2; fixedDim = 1; }      // XZ plane, fixed Y
    else if (axis == 1) { dim1 = 1; dim2 = 2; fixedDim = 0; } // YZ plane, fixed X
    else { dim1 = 0; dim2 = 1; fixedDim = 2; }                 // XY plane, fixed Z
    
    int size1 = (dim1 == 0 || dim2 == 0) ? CHUNK_SIZE : (dim1 == 1 || dim2 == 1) ? CHUNK_HEIGHT : CHUNK_SIZE;
    int size2 = (dim1 == 0 || dim2 == 0) ? CHUNK_SIZE : (dim1 == 1 || dim2 == 1) ? CHUNK_HEIGHT : CHUNK_SIZE;
    int fixedSize = (fixedDim == 0 || fixedDim == 2) ? CHUNK_SIZE : CHUNK_HEIGHT;
    
    // Greedy meshing: find largest rectangles
    for (int f = 0; f < fixedSize; f++) {
        for (int i = 0; i < size1; i++) {
            for (int j = 0; j < size2; ) {
                if (!mask[chunk.getIndex(
                    dim1 == 0 ? i : (dim1 == 1 ? j : f),
                    dim2 == 1 ? j : (dim2 == 0 ? i : f),
                    dim1 == 2 ? i : (dim2 == 2 ? j : f)
                )]) {
                    j++;
                    continue;
                }
                
                // Found a starting voxel, now expand
                int x_start, y_start, z_start;
                if (dim1 == 0) { x_start = i; y_start = (dim2 == 1) ? j : f; z_start = (dim2 == 2) ? j : f; }
                else if (dim1 == 1) { x_start = (dim2 == 0) ? j : f; y_start = i; z_start = (dim2 == 2) ? j : f; }
                else { x_start = f; y_start = i; z_start = j; }
                
                BlockType blockType = chunk.getBlock(x_start, y_start, z_start);
                
                // Expand along dimension 2 (width)
                int width = 1;
                while (j + width < size2) {
                    int x_next, y_next, z_next;
                    if (dim1 == 0) { x_next = i; y_next = (dim2 == 1) ? j + width : f; z_next = (dim2 == 2) ? j + width : f; }
                    else if (dim1 == 1) { x_next = (dim2 == 0) ? j + width : f; y_next = i; z_next = (dim2 == 2) ? j + width : f; }
                    else { x_next = f; y_next = i; z_next = j + width; }
                    
                    int idx = chunk.getIndex(x_next, y_next, z_next);
                    if (!mask[idx] || merged[idx]) break;
                    if (chunk.getBlock(x_next, y_next, z_next) != blockType) break;
                    width++;
                }
                
                // Expand along dimension 1 (height)
                int height = 1;
                bool canExpand = true;
                while (i + height < size1 && canExpand) {
                    for (int w = 0; w < width; w++) {
                        int x_check, y_check, z_check;
                        if (dim1 == 0) { x_check = i + height; y_check = (dim2 == 1) ? j + w : f; z_check = (dim2 == 2) ? j + w : f; }
                        else if (dim1 == 1) { x_check = (dim2 == 0) ? j + w : f; y_check = i + height; z_check = (dim2 == 2) ? j + w : f; }
                        else { x_check = f; y_check = i + height; z_check = j + w; }
                        
                        int idx = chunk.getIndex(x_check, y_check, z_check);
                        if (!mask[idx] || merged[idx]) {
                            canExpand = false;
                            break;
                        }
                        if (chunk.getBlock(x_check, y_check, z_check) != blockType) {
                            canExpand = false;
                            break;
                        }
                    }
                    if (canExpand) height++;
                }
                
                // Mark voxels as merged
                for (int h = 0; h < height; h++) {
                    for (int w = 0; w < width; w++) {
                        int x_m, y_m, z_m;
                        if (dim1 == 0) { x_m = i + h; y_m = (dim2 == 1) ? j + w : f; z_m = (dim2 == 2) ? j + w : f; }
                        else if (dim1 == 1) { x_m = (dim2 == 0) ? j + w : f; y_m = i + h; z_m = (dim2 == 2) ? j + w : f; }
                        else { x_m = f; y_m = i + h; z_m = j + w; }
                        
                        merged[chunk.getIndex(x_m, y_m, z_m)] = true;
                    }
                }
                
                // Calculate AO for corners
                std::array<uint8_t, 4> aoValues = {255, 255, 255, 255};
                
                // Add merged quad
                float fx = static_cast<float>(x_start);
                float fy = static_cast<float>(y_start);
                float fz = static_cast<float>(z_start);
                
                AddMergedQuad(vertices, fx, fy, fz, static_cast<float>(width), static_cast<float>(height),
                             axis, positiveDir, blockType, aoValues);
                
                j += width;
            }
        }
    }
    
    delete[] mask;
    delete[] merged;
}

void VoxelMesher::AddMergedQuad(
    std::vector<Vertex>& vertices,
    float x, float y, float z,
    float width, float height,
    int axis,
    bool positiveDir,
    BlockType blockType,
    const std::array<uint8_t, 4>& aoValues
) {
    // Calculate normal and offset based on axis and direction
    float nx = 0, ny = 0, nz = 0;
    float ox = 0, oy = 0, oz = 0;
    
    if (axis == 0) { // Y axis
        ny = positiveDir ? 1.0f : -1.0f;
        oy = positiveDir ? 1.0f : 0.0f;
    } else if (axis == 1) { // X axis
        nx = positiveDir ? 1.0f : -1.0f;
        ox = positiveDir ? 1.0f : 0.0f;
    } else { // Z axis
        nz = positiveDir ? 1.0f : -1.0f;
        oz = positiveDir ? 1.0f : 0.0f;
    }
    
    // Get texture UV coordinates
    float uBase, vBase, cellSize;
    int face = (axis == 0) ? (positiveDir ? FACE_TOP : FACE_BOTTOM) :
               (axis == 1) ? (positiveDir ? FACE_RIGHT : FACE_LEFT) :
                            (positiveDir ? FACE_FRONT : FACE_BACK);
    
    GetTextureUV(blockType, face, uBase, vBase, cellSize);
    
    // Calculate vertex positions and UVs for the merged quad
    float w = width;
    float h = height;
    
    // Determine which dimensions vary based on axis
    float x1 = x, x2 = x + w;
    float y1 = y, y2 = y + h;
    float z1 = z, z2 = z + w;
    
    // Vertices for the quad (2 triangles)
    // Order depends on axis to maintain correct winding
    
    if (axis == 0) { // Y faces (XZ plane)
        vertices.push_back({x1,      y + oy, z1,      nx, ny, nz, uBase,           vBase,           aoValues[0], aoValues[0], aoValues[0], 255});
        vertices.push_back({x2,      y + oy, z1,      nx, ny, nz, uBase + w*cellSize, vBase,           aoValues[1], aoValues[1], aoValues[1], 255});
        vertices.push_back({x1,      y + oy, z2,      nx, ny, nz, uBase,           vBase + h*cellSize, aoValues[2], aoValues[2], aoValues[2], 255});
        vertices.push_back({x1,      y + oy, z2,      nx, ny, nz, uBase,           vBase + h*cellSize, aoValues[2], aoValues[2], aoValues[2], 255});
        vertices.push_back({x2,      y + oy, z1,      nx, ny, nz, uBase + w*cellSize, vBase,           aoValues[1], aoValues[1], aoValues[1], 255});
        vertices.push_back({x2,      y + oy, z2,      nx, ny, nz, uBase + w*cellSize, vBase + h*cellSize, aoValues[3], aoValues[3], aoValues[3], 255});
    } else if (axis == 1) { // X faces (YZ plane)
        vertices.push_back({x + ox, y1,      z1,      nx, ny, nz, uBase,           vBase,           aoValues[0], aoValues[0], aoValues[0], 255});
        vertices.push_back({x + ox, y1,      z2,      nx, ny, nz, uBase + w*cellSize, vBase,           aoValues[1], aoValues[1], aoValues[1], 255});
        vertices.push_back({x + ox, y2,      z1,      nx, ny, nz, uBase,           vBase + h*cellSize, aoValues[2], aoValues[2], aoValues[2], 255});
        vertices.push_back({x + ox, y2,      z1,      nx, ny, nz, uBase,           vBase + h*cellSize, aoValues[2], aoValues[2], aoValues[2], 255});
        vertices.push_back({x + ox, y1,      z2,      nx, ny, nz, uBase + w*cellSize, vBase,           aoValues[1], aoValues[1], aoValues[1], 255});
        vertices.push_back({x + ox, y2,      z2,      nx, ny, nz, uBase + w*cellSize, vBase + h*cellSize, aoValues[3], aoValues[3], aoValues[3], 255});
    } else { // Z faces (XY plane)
        vertices.push_back({x1,      y1,      z + oz, nx, ny, nz, uBase,           vBase,           aoValues[0], aoValues[0], aoValues[0], 255});
        vertices.push_back({x2,      y1,      z + oz, nx, ny, nz, uBase + w*cellSize, vBase,           aoValues[1], aoValues[1], aoValues[1], 255});
        vertices.push_back({x1,      y2,      z + oz, nx, ny, nz, uBase,           vBase + h*cellSize, aoValues[2], aoValues[2], aoValues[2], 255});
        vertices.push_back({x1,      y2,      z + oz, nx, ny, nz, uBase,           vBase + h*cellSize, aoValues[2], aoValues[2], aoValues[2], 255});
        vertices.push_back({x2,      y1,      z + oz, nx, ny, nz, uBase + w*cellSize, vBase,           aoValues[1], aoValues[1], aoValues[1], 255});
        vertices.push_back({x2,      y2,      z + oz, nx, ny, nz, uBase + w*cellSize, vBase + h*cellSize, aoValues[3], aoValues[3], aoValues[3], 255});
    }
}

uint8_t VoxelMesher::ComputeAO(
    const Chunk& chunk,
    int x, int y, int z,
    int axis,
    bool positiveDir,
    int cornerIndex,
    const std::unordered_map<long long, Chunk>* neighborChunks
) {
    // Simple AO calculation - samples neighboring voxels
    // Corner indices: 0=bottom-left, 1=bottom-right, 2=top-left, 3=top-right
    
    int ox = 0, oy = 0, oz = 0;
    
    // Determine offset based on corner and axis
    if (axis == 0) { // Y faces
        if (cornerIndex == 0 || cornerIndex == 2) ox = -1;
        else ox = 1;
        if (cornerIndex == 0 || cornerIndex == 1) oz = -1;
        else oz = 1;
    } else if (axis == 1) { // X faces
        if (cornerIndex == 0 || cornerIndex == 2) oy = -1;
        else oy = 1;
        if (cornerIndex == 0 || cornerIndex == 1) oz = -1;
        else oz = 1;
    } else { // Z faces
        if (cornerIndex == 0 || cornerIndex == 2) ox = -1;
        else ox = 1;
        if (cornerIndex == 0 || cornerIndex == 1) oy = -1;
        else oy = 1;
    }
    
    // Sample neighbors for AO
    int occlusion = 0;
    
    // Check adjacent voxels
    if (chunk.isValid(x + ox, y, z) && chunk.getBlock(x + ox, y, z) != BlockType::AIR) occlusion++;
    if (chunk.isValid(x, y + oy, z) && chunk.getBlock(x, y + oy, z) != BlockType::AIR) occlusion++;
    if (chunk.isValid(x + ox, y + oy, z) && chunk.getBlock(x + ox, y + oy, z) != BlockType::AIR) occlusion++;
    
    // Map occlusion to AO value (more occlusion = darker)
    // Using simple linear mapping: 0=255, 1=200, 2=150, 3=100
    uint8_t aoMap[] = {255, 200, 150, 100};
    return aoMap[occlusion];
}

void VoxelMesher::GetTextureUV(BlockType blockType, int face, float& u, float& v, float& cellSize) {
    const BlockProperties& props = GetBlockProperties(blockType);
    
    // Select texture index based on face
    int texIndex = props.texSide;
    if (face == FACE_TOP) texIndex = props.texTop;
    else if (face == FACE_BOTTOM) texIndex = props.texBottom;
    
    // Texture atlas is 4x4 grid (16 textures)
    cellSize = 0.25f;
    
    int texRow = texIndex / 4;
    int texCol = texIndex % 4;
    
    u = texCol * cellSize;
    v = texRow * cellSize;
}
