#include "voxel_mesher.h"
#include <cmath>

// Calculate texture UVs, lighting, and colors for a face
void CalculateLightingAndTexture(BlockType blockType, Face face, int axis, int sign, int du, int dv,
                                    float& uBase, float& vBase, float& cellSize,
                                    uint8_t& cr, uint8_t& cg, uint8_t& cb, uint8_t& ca) {
    const BlockProperties& props = GetBlockProperties(blockType);
    
    // Get texture index
    int texIndex = props.texSide;
    if (face == FACE_TOP) texIndex = props.texTop;
    else if (face == FACE_BOTTOM) texIndex = props.texBottom;
    
    // Calculate texture coordinates
    cellSize = 0.25f;
    int row = texIndex / 4;
    int col = texIndex % 4;
    uBase = col * cellSize;
    vBase = row * cellSize;
    
    // Calculate lighting
    float nx = 0, ny = 0, nz = 0;
    if (axis == 0) nx = (float)sign;
    if (axis == 1) ny = (float)sign;
    if (axis == 2) nz = (float)sign;
    
    // Sun direction
    float sunX = 0.5f, sunY = 0.8f, sunZ = 0.3f;
    float sunLen = sqrtf(sunX*sunX + sunY*sunY + sunZ*sunZ);
    sunX /= sunLen; sunY /= sunLen; sunZ /= sunLen;
    const float AMBIENT = 0.3f;
    
    float lightIntensity = nx * sunX + ny * sunY + nz * sunZ;
    lightIntensity = AMBIENT + (1.0f - AMBIENT) * fmaxf(0.0f, lightIntensity);
    
    // Apply lighting to base colors
    float baseR = props.color.r / 255.0f;
    float baseG = props.color.g / 255.0f;
    float baseB = props.color.b / 255.0f;
    cr = (uint8_t)(baseR * lightIntensity * 255.0f);
    cg = (uint8_t)(baseG * lightIntensity * 255.0f);
    cb = (uint8_t)(baseB * lightIntensity * 255.0f);
    ca = props.color.a;
}

void VoxelMesher::GetTextureUV(BlockType type, Face face, float& u, float& v, float& cellSize) {
    const BlockProperties& props = GetBlockProperties(type);
    int texIndex = props.texSide;
    if (face == FACE_TOP) texIndex = props.texTop;
    else if (face == FACE_BOTTOM) texIndex = props.texBottom;
    
    cellSize = 0.25f;
    int row = texIndex / 4;
    int col = texIndex % 4;
    u = col * cellSize;
    v = row * cellSize;
}
