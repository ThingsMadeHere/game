#include "terrain.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>

void Chunk::GenerateDensityTexture() {
    // Only regenerate if dirty or not generated yet
    if (!densityTextureGenerated && densityTexture.id > 0) {
        return; // Already generated and not dirty
    }
    
    // Create a 3D texture where black = air, white = block
    int textureSize = CHUNK_SIZE;     // Now 32
    int heightSize = CHUNK_HEIGHT;   // Now 32
    
    // Allocate memory for 3D texture data (grayscale: black=air, white=block)
    unsigned char* textureData = (unsigned char*)malloc(textureSize * heightSize * textureSize);
    
    for (int x = 0; x < textureSize; x++) {
        for (int y = 0; y < heightSize; y++) {
            for (int z = 0; z < textureSize; z++) {
                int index = x * heightSize * textureSize + y * textureSize + z;
                
                // Get density and convert to grayscale (0=black air, 255=white block)
                float density = GetDensity(x, y, z);
                
                if (density > 0.0f) {
                    // Solid block - white (255)
                    textureData[index] = 255;
                } else {
                    // Air - black (0)
                    textureData[index] = 0;
                }
            }
        }
    }
    
    // Create a 2D texture that represents the 3D data (for simplicity)
    // In a real implementation, you'd use a 3D texture or multiple 2D slices
    Image textureImage = {
        textureData,
        textureSize,
        heightSize * textureSize, // Stack all slices vertically
        1,
        PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    };
    
    // Unload old texture if exists
    if (densityTexture.id > 0) {
        UnloadTexture(densityTexture);
    }
    
    densityTexture = LoadTextureFromImage(textureImage);
    UnloadImage(textureImage);
    
    densityTextureGenerated = true; // Mark as generated
    printf("Generated density texture for chunk (%d,%d,%d): %dx%dx%d (black=air, white=block)\n", 
           cx, cy, cz, textureSize, heightSize, textureSize);
}
