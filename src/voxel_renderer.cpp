#include "voxel_renderer.h"
#include <cstdio>

// Block colors for different materials
const Color VoxelRenderer::blockColors[12] = {
    BLACK,      // AIR (0)
    GRAY,       // STONE (1)
    BROWN,      // DIRT (2)
    GREEN,      // GRASS (3)
    YELLOW,     // SAND (4)
    BLUE,       // WATER (5)
    BEIGE,      // WOOD (6)
    DARKGREEN,  // LEAVES (7)
    DARKGRAY,   // COAL (8)
    LIGHTGRAY,  // IRON (9)
    GOLD,       // GOLD (10)
    WHITE       // DIAMOND (11)
};

void VoxelRenderer::Init() {
    // Initialize any needed resources
    printf("Voxel renderer initialized with %d block types\n", 12);
}

Color VoxelRenderer::GetBlockColor(unsigned char material) {
    if (material < 12) {
        return blockColors[material];
    }
    return WHITE; // Default for unknown materials
}

void VoxelRenderer::RenderChunkFromDensity(const Chunk& chunk) {
    // Render voxels by reading the density data and placing colored blocks
    int blocksRendered = 0;
    
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                // Check if this voxel is solid (density > 0)
                if (chunk.GetDensity(x, y, z) > 0.0f) {
                    // Get the material type
                    unsigned char material = chunk.GetMaterial(x, y, z);
                    
                    // Get the color for this material
                    Color blockColor = GetBlockColor(material);
                    
                    // Calculate world position
                    Vector3 blockPos = {
                        (float)chunk.cx * CHUNK_SIZE + x * VOXEL_SIZE,
                        (float)chunk.cy * CHUNK_HEIGHT + y * VOXEL_SIZE,
                        (float)chunk.cz * CHUNK_SIZE + z * VOXEL_SIZE
                    };
                    
                    // Draw the block
                    DrawCube(blockPos, VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE, blockColor);
                    blocksRendered++;
                }
            }
        }
    }
    
    printf("Rendered chunk (%d,%d,%d): %d blocks from density texture\n", 
           chunk.cx, chunk.cy, chunk.cz, blocksRendered);
}
