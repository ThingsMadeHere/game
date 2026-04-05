#pragma once
#include "raylib.h"
#include <vector>
#include <unordered_map>
#include <cstring>

// Constants - optimized for performance
const int CHUNK_SIZE = 16;        // Smaller chunks for faster generation
const int CHUNK_HEIGHT = 64;      // Taller chunks for better vertical coverage
const int RENDER_DISTANCE = 3;    // Conservative render distance
const float VOXEL_SIZE = 1.0f;    // Standard voxel size (1 unit = 1 block)

// Material/Block types
enum class BlockType : unsigned char {
    AIR = 0,
    GRASS = 1,
    DIRT = 2,
    STONE = 3,
    WOOD = 4,
    LEAVES = 5,
    SAND = 6,
    WATER = 7,
    BRICK = 8,
    METAL = 9,
    GLASS = 10,
    SNOW = 11,
    ICE = 12,
    CLAY = 13,
    GRAVEL = 14,
    COUNT
};

// Block properties with texture coordinates
struct BlockProperties {
    Color color;
    bool transparent;
    bool solid;           // Is this a solid block?
    float hardness;
    int texTop;           // Texture index for top face
    int texBottom;        // Texture index for bottom face
    int texSide;          // Texture index for side faces
};

// Get properties for a block type
inline BlockProperties GetBlockProperties(BlockType type) {
    switch (type) {
        case BlockType::GRASS:  return {{34, 139, 34, 255}, false, true, 1.0f, 0, 2, 1};
        case BlockType::DIRT:   return {{139, 69, 19, 255}, false, true, 0.8f, 2, 2, 2};
        case BlockType::STONE:  return {{128, 128, 128, 255}, false, true, 3.0f, 3, 3, 3};
        case BlockType::WOOD:   return {{139, 90, 43, 255}, false, true, 2.0f, 4, 4, 4};
        case BlockType::LEAVES: return {{34, 100, 34, 200}, true, true, 0.3f, 5, 5, 5};
        case BlockType::SAND:   return {{238, 203, 173, 255}, false, true, 0.5f, 6, 6, 6};
        case BlockType::WATER:  return {{65, 105, 225, 180}, true, false, 0.0f, 7, 7, 7};
        case BlockType::BRICK:  return {{178, 34, 34, 255}, false, true, 2.5f, 8, 8, 8};
        case BlockType::METAL:  return {{192, 192, 192, 255}, false, true, 5.0f, 9, 9, 9};
        case BlockType::GLASS:  return {{200, 200, 255, 100}, true, true, 0.5f, 10, 10, 10};
        case BlockType::SNOW:   return {{255, 250, 250, 255}, false, true, 0.5f, 11, 2, 11};
        case BlockType::ICE:    return {{200, 220, 255, 150}, true, true, 0.8f, 12, 12, 12};
        case BlockType::CLAY:   return {{180, 160, 140, 255}, false, true, 1.2f, 13, 13, 13};
        case BlockType::GRAVEL: return {{120, 110, 100, 255}, false, true, 0.6f, 14, 14, 14};
        default:                return {{255, 0, 255, 255}, false, true, 1.0f, 0, 0, 0};
    }
}

// Voxel data - simplified to just block type
struct Voxel {
    BlockType type;
    unsigned char light;      // Light level (0-15)
    
    Voxel() : type(BlockType::AIR), light(15) {}
    Voxel(BlockType t) : type(t), light(15) {}
};

// Face indices for culling
enum Face {
    FACE_RIGHT = 0,   // +X
    FACE_LEFT = 1,    // -X
    FACE_TOP = 2,     // +Y
    FACE_BOTTOM = 3,  // -Y
    FACE_FRONT = 4,   // +Z
    FACE_BACK = 5     // -Z
};

// Chunk data - uses flat array for better cache performance
struct Chunk {
    int cx, cy, cz;
    Voxel voxels[CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE]; // Flat array
    Mesh mesh = {0};
    bool needsUpdate = true;
    bool meshGenerated = false;
    bool initialized = false;
    
    Chunk(int x, int y, int z) : cx(x), cy(y), cz(z), meshGenerated(false), initialized(false) {
        memset(voxels, 0, sizeof(voxels));
    }
    
    // Copy constructor needed for unordered_map
    Chunk(const Chunk& other) : cx(other.cx), cy(other.cy), cz(other.cz), 
                                 meshGenerated(false), initialized(false) {
        memcpy(voxels, other.voxels, sizeof(voxels));
        if (other.mesh.vertexCount > 0 && other.mesh.vertices) {
            // Don't copy mesh - regenerate it
            mesh = {0};
            needsUpdate = true;
        } else {
            mesh = {0};
            needsUpdate = other.needsUpdate;
        }
    }
    
    Chunk& operator=(const Chunk& other) {
        if (this != &other) {
            cx = other.cx; cy = other.cy; cz = other.cz;
            memcpy(voxels, other.voxels, sizeof(voxels));
            meshGenerated = false;
            initialized = false;
            if (other.mesh.vertexCount > 0) {
                mesh = {0};
                needsUpdate = true;
            } else {
                mesh = {0};
                needsUpdate = other.needsUpdate;
            }
        }
        return *this;
    }
    
    inline int getIndex(int x, int y, int z) const {
        return x + z * CHUNK_SIZE + y * CHUNK_SIZE * CHUNK_SIZE;
    }
    
    inline bool isValid(int x, int y, int z) const {
        return x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE;
    }
    
    Voxel& getVoxel(int x, int y, int z) {
        static Voxel air(BlockType::AIR);
        if (!isValid(x, y, z)) return air;
        return voxels[getIndex(x, y, z)];
    }
    
    const Voxel& getVoxel(int x, int y, int z) const {
        static Voxel air(BlockType::AIR);
        if (!isValid(x, y, z)) return air;
        return voxels[getIndex(x, y, z)];
    }
    
    void setVoxel(int x, int y, int z, BlockType type) {
        if (isValid(x, y, z)) {
            voxels[getIndex(x, y, z)].type = type;
            needsUpdate = true;
            meshGenerated = false;
        }
    }
    
    BlockType getBlock(int x, int y, int z) const {
        if (!isValid(x, y, z)) return BlockType::AIR;
        return voxels[getIndex(x, y, z)].type;
    }
    
    // Check if a face should be rendered (neighbor is air or transparent)
    bool shouldRenderFace(int x, int y, int z, Face face, 
                          const std::unordered_map<long long, Chunk>* neighborChunks = nullptr) const {
        int nx = x, ny = y, nz = z;
        
        switch (face) {
            case FACE_RIGHT:  nx++; break;
            case FACE_LEFT:   nx--; break;
            case FACE_TOP:    ny++; break;
            case FACE_BOTTOM: ny--; break;
            case FACE_FRONT:  nz++; break;
            case FACE_BACK:   nz--; break;
        }
        
        // Check if neighbor is in this chunk
        if (isValid(nx, ny, nz)) {
            BlockType neighbor = getBlock(nx, ny, nz);
            return neighbor == BlockType::AIR || GetBlockProperties(neighbor).transparent;
        }
        
        // Neighbor is in another chunk - check if it exists and is air
        if (neighborChunks) {
            int ncx = cx, ncy = cy, ncz = cz;
            int localX = nx, localY = ny, localZ = nz;
            
            // Wrap coordinates to neighbor chunk
            if (localX < 0) { ncx--; localX += CHUNK_SIZE; }
            else if (localX >= CHUNK_SIZE) { ncx++; localX -= CHUNK_SIZE; }
            
            if (localY < 0) { ncy--; localY += CHUNK_HEIGHT; }
            else if (localY >= CHUNK_HEIGHT) { ncy++; localY -= CHUNK_HEIGHT; }
            
            if (localZ < 0) { ncz--; localZ += CHUNK_SIZE; }
            else if (localZ >= CHUNK_SIZE) { ncz++; localZ -= CHUNK_SIZE; }
            
            long long key = (((long long)ncx) << 32) | (((long long)ncy) << 16) | ((long long)ncz);
            auto it = neighborChunks->find(key);
            if (it != neighborChunks->end()) {
                BlockType neighbor = it->second.getBlock(localX, localY, localZ);
                return neighbor == BlockType::AIR || GetBlockProperties(neighbor).transparent;
            }
        }
        
        // No neighbor chunk - assume air (render the face)
        return true;
    }
};

// Hash function for chunk keys - use simple long long encoding
inline long long makeChunkKey(int cx, int cy, int cz) {
    return (((long long)cx) << 32) | (((long long)cy) << 16) | ((long long)cz);
}
