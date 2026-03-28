// src/MarchingCubes.cpp
#include "MarchingCubes.h"
#include <cstdio>

// Edge table for marching cubes
const int MarchingCubes::edgeTable[256] = {
    0x0,  0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x99,  0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x33,  0x13a, 0x636, 0x73f, 0x435, 0x53c,
    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1f3, 0xfa,  0x7f6, 0x6ff, 0x5f5, 0x4fc,
    0xafc, 0xbf5, 0x8ff, 0x9f6, 0xefa, 0xff3, 0xcf9, 0xdf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55,  0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc,
    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
    0xcc,  0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
    0x15c, 0x55,  0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xdf0, 0xcf9, 0xff3, 0xefa, 0x9f6, 0x8ff, 0xbf5, 0xafc,
    0x4fc, 0x5f5, 0x6ff, 0x7f6, 0xfa,  0x1f3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33,  0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99,  0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};

// Triangle table for marching cubes
const int MarchingCubes::triTable[256][16] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
    {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
    {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
    {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
    {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
    {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
    {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
    {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
    {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
    {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
    {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
    {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
    {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
    {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
    {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
    {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
    {9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
    {1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
    {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
    {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
    {2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
    {7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
    {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
    {11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
    {9, 7, 8, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 7, 9, 7, 2, 9, 2, 0, 7, 2, 11, -1, -1, -1, -1},
    {2, 3, 11, 2, 11, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1},
    {11, 2, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 1, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
    {9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 0, 6, 5, 6, 2, 3, 6, 2, 8, 6, 0, -1, -1, -1, -1},
    {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
    {2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
    {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
    {3, 6, 5, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {6, 5, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 3, 0, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 0, 9, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {5, 6, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {5, 6, 4, 1, 2, 10, 3, 0, 9, -1, -1, -1, -1, -1, -1, -1},
    {6, 4, 5, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {6, 4, 5, 2, 10, 1, 3, 0, 9, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
    {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {4, 9, 5, 1, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
    {2, 6, 1, 6, 5, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 2, 6, 1, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
    {1, 6, 10, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 10, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {6, 1, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 6, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 4, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 6, 1, 9, 0, 1, 0, 8, 9, 8, 0, -1, -1, -1, -1},
    {1, 2, 10, 4, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 1, 2, 10, 4, 7, 6, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 9, 0, 2, 8, 4, 7, 6, -1, -1, -1, -1, -1, -1},
    {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, 6, 4, 7, -1},
    {8, 4, 7, 6, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 4, 7, 11, 2, 4, 2, 0, 4, 6, 4, 7, -1, -1, -1, -1},
    {6, 4, 7, 0, 8, 3, 9, 1, 2, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 1, 9, 1, 0, 8, 4, 7, 6, -1, -1, -1, -1, -1, -1},
    {3, 11, 2, 8, 4, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 11, 2, 0, 8, 1, 8, 7, 1, 8, 6, 7, 4, 6, 7, -1},
    {10, 6, 4, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {6, 4, 10, 0, 8, 3, 11, 2, 1, -1, -1, -1, -1, -1, -1, -1},
    {6, 4, 10, 9, 0, 1, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
    {4, 10, 6, 1, 9, 0, 9, 8, 0, 9, 11, 8, 2, 11, 8, -1},
    {4, 7, 6, 5, 9, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 6, 5, 9, 8, 3, 0, 1, -1, -1, -1, -1, -1, -1, -1},
    {5, 9, 8, 0, 1, 7, 1, 6, 7, 1, 4, 6, -1, -1, -1, -1},
    {5, 9, 8, 1, 6, 7, 1, 4, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 9, 8, 10, 1, 6, 1, 7, 6, 1, 2, 7, -1, -1, -1, -1},
    {10, 1, 6, 1, 7, 6, 5, 9, 8, 3, 0, 2, -1, -1, -1, -1},
    {5, 9, 8, 10, 6, 4, 6, 7, 4, 10, 2, 6, 2, 1, 6, -1},
    {6, 4, 7, 2, 10, 5, 2, 5, 3, 5, 10, 8, 5, 8, 9, -1},
    {11, 3, 2, 5, 9, 8, 4, 7, 6, -1, -1, -1, -1, -1, -1, -1},
    {11, 3, 2, 5, 9, 8, 4, 7, 6, 1, 0, 10, -1, -1, -1, -1},
    {5, 9, 8, 11, 3, 2, 0, 1, 7, 1, 6, 7, 1, 4, 6, -1},
    {11, 3, 2, 5, 9, 8, 1, 6, 7, 1, 4, 6, -1, -1, -1, -1},
    {5, 9, 8, 11, 3, 2, 6, 4, 7, -1, -1, -1, -1, -1, -1, -1},
    {11, 3, 2, 5, 9, 8, 6, 4, 7, 10, 1, 0, -1, -1, -1, -1},
    {5, 9, 8, 11, 3, 2, 10, 1, 6, 1, 7, 6, 1, 4, 7, -1},
    {11, 3, 2, 5, 9, 8, 10, 6, 4, 6, 7, 4, -1, -1, -1, -1},
    {10, 6, 4, 11, 3, 2, 5, 9, 8, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 10, 6, 4, 5, 9, 8, 1, 2, 11, -1, -1, -1, -1},
    {10, 6, 4, 11, 3, 2, 9, 0, 1, 5, 9, 1, -1, -1, -1, -1},
    {10, 6, 4, 11, 3, 2, 5, 9, 1, -1, -1, -1, -1, -1, -1, -1},
    {10, 6, 4, 11, 3, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};

float MarchingCubes::GetDensity(float x, float y, float z,
                                std::function<float(float, float, float)> densityFunc) {
    return densityFunc(x, y, z);
}

Vertex MarchingCubes::InterpolateVertex(const Vertex& v1, const Vertex& v2, float val1, float val2, float isoLevel) {
    Vertex result;
    float t = (isoLevel - val1) / (val2 - val1);
    
    result.x = v1.x + t * (v2.x - v1.x);
    result.y = v1.y + t * (v2.y - v1.y);
    result.z = v1.z + t * (v2.z - v1.z);
    
    // Default color (can be modified based on height or other factors)
    result.r = 0.5f;
    result.g = 0.8f;
    result.b = 0.3f;
    
    return result;
}

void MarchingCubes::CalculateNormal(Vertex& vertex, float x, float y, float z, 
                                   std::function<float(float, float, float)> densityFunc, float epsilon) {
    float dx = densityFunc(x + epsilon, y, z) - densityFunc(x - epsilon, y, z);
    float dy = densityFunc(x, y + epsilon, z) - densityFunc(x, y - epsilon, z);
    float dz = densityFunc(x, y, z + epsilon) - densityFunc(x, y, z - epsilon);
    
    // Normalize the gradient
    float length = sqrt(dx * dx + dy * dy + dz * dz);
    if (length > 0.0f) {
        vertex.nx = -dx / length;
        vertex.ny = -dy / length;
        vertex.nz = -dz / length;
    } else {
        vertex.nx = 0.0f;
        vertex.ny = 1.0f;
        vertex.nz = 0.0f;
    }
}

void MarchingCubes::AddTriangle(MeshData& mesh, const Vertex& v1, const Vertex& v2, const Vertex& v3) {
    unsigned int baseIndex = mesh.vertices.size();
    
    mesh.vertices.push_back(v1);
    mesh.vertices.push_back(v2);
    mesh.vertices.push_back(v3);
    
    mesh.indices.push_back(baseIndex);
    mesh.indices.push_back(baseIndex + 1);
    mesh.indices.push_back(baseIndex + 2);
}

MeshData MarchingCubes::GenerateChunk(
    float centerX, float centerY, float centerZ,
    float chunkSize, int resolution,
    std::function<float(float, float, float)> densityFunc
) {
    MeshData mesh;
    
    float stepSize = chunkSize / resolution;
    float halfChunk = chunkSize * 0.5f;
    float isoLevel = 0.0f;
    
    printf("Generating chunk at (%.1f, %.1f, %.1f) size %.1f res %d\n",
           centerX, centerY, centerZ, chunkSize, resolution);
    
    // Iterate through each cube in chunk
    for (int x = 0; x < resolution; x++) {
        for (int y = 0; y < resolution; y++) {
            for (int z = 0; z < resolution; z++) {
                // Calculate cube corner positions
                float px = centerX - halfChunk + x * stepSize;
                float py = centerY - halfChunk + y * stepSize;
                float pz = centerZ - halfChunk + z * stepSize;
                
                // Get density values at cube corners
                float density[8];
                density[0] = GetDensity(px, py, pz, densityFunc);
                density[1] = GetDensity(px + stepSize, py, pz, densityFunc);
                density[2] = GetDensity(px + stepSize, py, pz + stepSize, densityFunc);
                density[3] = GetDensity(px, py, pz + stepSize, densityFunc);
                density[4] = GetDensity(px, py + stepSize, pz, densityFunc);
                density[5] = GetDensity(px + stepSize, py + stepSize, pz, densityFunc);
                density[6] = GetDensity(px + stepSize, py + stepSize, pz + stepSize, densityFunc);
                density[7] = GetDensity(px, py + stepSize, pz + stepSize, densityFunc);
                
                // Calculate cube configuration
                int cubeIndex = 0;
                for (int i = 0; i < 8; i++) {
                    if (density[i] < isoLevel) {
                        cubeIndex |= (1 << i);
                    }
                }
                
                // Skip if cube is completely inside or outside
                if (edgeTable[cubeIndex] == 0) {
                    continue;
                }
                
                // Calculate vertices for this cube
                Vertex vertices[12];
                
                // Edge 0: between vertices 0 and 1
                if (edgeTable[cubeIndex] & 1) {
                    Vertex v0{px, py, pz};
                    Vertex v1{px + stepSize, py, pz};
                    vertices[0] = InterpolateVertex(v0, v1, density[0], density[1], isoLevel);
                    CalculateNormal(vertices[0], vertices[0].x, vertices[0].y, vertices[0].z, densityFunc);
                }
                
                // Edge 1: between vertices 1 and 2
                if (edgeTable[cubeIndex] & 2) {
                    Vertex v0{px + stepSize, py, pz};
                    Vertex v1{px + stepSize, py, pz + stepSize};
                    vertices[1] = InterpolateVertex(v0, v1, density[1], density[2], isoLevel);
                    CalculateNormal(vertices[1], vertices[1].x, vertices[1].y, vertices[1].z, densityFunc);
                }
                
                // Edge 2: between vertices 2 and 3
                if (edgeTable[cubeIndex] & 4) {
                    Vertex v0{px + stepSize, py, pz + stepSize};
                    Vertex v1{px, py, pz + stepSize};
                    vertices[2] = InterpolateVertex(v0, v1, density[2], density[3], isoLevel);
                    CalculateNormal(vertices[2], vertices[2].x, vertices[2].y, vertices[2].z, densityFunc);
                }
                
                // Edge 3: between vertices 3 and 0
                if (edgeTable[cubeIndex] & 8) {
                    Vertex v0{px, py, pz + stepSize};
                    Vertex v1{px, py, pz};
                    vertices[3] = InterpolateVertex(v0, v1, density[3], density[0], isoLevel);
                    CalculateNormal(vertices[3], vertices[3].x, vertices[3].y, vertices[3].z, densityFunc);
                }
                
                // Edge 4: between vertices 4 and 5
                if (edgeTable[cubeIndex] & 16) {
                    Vertex v0{px, py + stepSize, pz};
                    Vertex v1{px + stepSize, py + stepSize, pz};
                    vertices[4] = InterpolateVertex(v0, v1, density[4], density[5], isoLevel);
                    CalculateNormal(vertices[4], vertices[4].x, vertices[4].y, vertices[4].z, densityFunc);
                }
                
                // Edge 5: between vertices 5 and 6
                if (edgeTable[cubeIndex] & 32) {
                    Vertex v0{px + stepSize, py + stepSize, pz};
                    Vertex v1{px + stepSize, py + stepSize, pz + stepSize};
                    vertices[5] = InterpolateVertex(v0, v1, density[5], density[6], isoLevel);
                    CalculateNormal(vertices[5], vertices[5].x, vertices[5].y, vertices[5].z, densityFunc);
                }
                
                // Edge 6: between vertices 6 and 7
                if (edgeTable[cubeIndex] & 64) {
                    Vertex v0{px + stepSize, py + stepSize, pz + stepSize};
                    Vertex v1{px, py + stepSize, pz + stepSize};
                    vertices[6] = InterpolateVertex(v0, v1, density[6], density[7], isoLevel);
                    CalculateNormal(vertices[6], vertices[6].x, vertices[6].y, vertices[6].z, densityFunc);
                }
                
                // Edge 7: between vertices 7 and 4
                if (edgeTable[cubeIndex] & 128) {
                    Vertex v0{px, py + stepSize, pz + stepSize};
                    Vertex v1{px, py + stepSize, pz};
                    vertices[7] = InterpolateVertex(v0, v1, density[7], density[4], isoLevel);
                    CalculateNormal(vertices[7], vertices[7].x, vertices[7].y, vertices[7].z, densityFunc);
                }
                
                // Edge 8: between vertices 0 and 4
                if (edgeTable[cubeIndex] & 256) {
                    Vertex v0{px, py, pz};
                    Vertex v1{px, py + stepSize, pz};
                    vertices[8] = InterpolateVertex(v0, v1, density[0], density[4], isoLevel);
                    CalculateNormal(vertices[8], vertices[8].x, vertices[8].y, vertices[8].z, densityFunc);
                }
                
                // Edge 9: between vertices 1 and 5
                if (edgeTable[cubeIndex] & 512) {
                    Vertex v0{px + stepSize, py, pz};
                    Vertex v1{px + stepSize, py + stepSize, pz};
                    vertices[9] = InterpolateVertex(v0, v1, density[1], density[5], isoLevel);
                    CalculateNormal(vertices[9], vertices[9].x, vertices[9].y, vertices[9].z, densityFunc);
                }
                
                // Edge 10: between vertices 2 and 6
                if (edgeTable[cubeIndex] & 1024) {
                    Vertex v0{px + stepSize, py, pz + stepSize};
                    Vertex v1{px + stepSize, py + stepSize, pz + stepSize};
                    vertices[10] = InterpolateVertex(v0, v1, density[2], density[6], isoLevel);
                    CalculateNormal(vertices[10], vertices[10].x, vertices[10].y, vertices[10].z, densityFunc);
                }
                
                // Edge 11: between vertices 3 and 7
                if (edgeTable[cubeIndex] & 2048) {
                    Vertex v0{px, py, pz + stepSize};
                    Vertex v1{px, py + stepSize, pz + stepSize};
                    vertices[11] = InterpolateVertex(v0, v1, density[3], density[7], isoLevel);
                    CalculateNormal(vertices[11], vertices[11].x, vertices[11].y, vertices[11].z, densityFunc);
                }
                
                // Generate triangles using triangle table
                for (int i = 0; i < 16 && triTable[cubeIndex][i] != -1; i += 3) {
                    // Make sure we don't go out of bounds
                    if (i + 2 < 16 && triTable[cubeIndex][i] != -1 && 
                        triTable[cubeIndex][i + 1] != -1 && triTable[cubeIndex][i + 2] != -1) {
                        AddTriangle(mesh, 
                                   vertices[triTable[cubeIndex][i]],
                                   vertices[triTable[cubeIndex][i + 1]],
                                   vertices[triTable[cubeIndex][i + 2]]);
                    }
                }
            }
        }
    }
    
    printf("Generated mesh with %zu vertices and %zu triangles\n", 
           mesh.vertices.size(), mesh.indices.size() / 3);
    
    return mesh;
}
