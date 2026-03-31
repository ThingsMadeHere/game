#include "noise.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>

// Improved Perlin-like noise implementation
float Hash(float x, float y, float z) {
    // Better hash function for smoother noise
    int xi = (int)floorf(x) & 255;
    int yi = (int)floorf(y) & 255;
    int zi = (int)floorf(z) & 255;
    
    // Simple permutation table simulation
    int hash = (xi * 374761393 + yi * 668265263 + zi * 217460973) % 256;
    return (float)hash / 256.0f;
}

float SmoothNoise3D(float x, float y, float z) {
    // Get integer coordinates
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    int iz = (int)floorf(z);
    
    // Get fractional coordinates
    float fx = x - ix;
    float fy = y - iy;
    float fz = z - iz;
    
    // Get hash values for corners
    float h000 = Hash(ix, iy, iz);
    float h001 = Hash(ix, iy, iz + 1);
    float h010 = Hash(ix, iy + 1, iz);
    float h011 = Hash(ix, iy + 1, iz + 1);
    float h100 = Hash(ix + 1, iy, iz);
    float h101 = Hash(ix + 1, iy, iz + 1);
    float h110 = Hash(ix + 1, iy + 1, iz);
    float h111 = Hash(ix + 1, iy + 1, iz + 1);
    
    // Smoothstep interpolation
    float u = fx * fx * (3.0f - 2.0f * fx);
    float v = fy * fy * (3.0f - 2.0f * fy);
    float w = fz * fz * (3.0f - 2.0f * fz);
    
    // Trilinear interpolation
    float x00 = h000 * (1.0f - u) + h100 * u;
    float x01 = h001 * (1.0f - u) + h101 * u;
    float x10 = h010 * (1.0f - u) + h110 * u;
    float x11 = h011 * (1.0f - u) + h111 * u;
    
    float y0 = x00 * (1.0f - v) + x10 * v;
    float y1 = x01 * (1.0f - v) + x11 * v;
    
    return y0 * (1.0f - w) + y1 * w;
}

float FBM(float x, float y, float z, int octaves) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 0.05f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        value += SmoothNoise3D(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return value / maxValue;
}
