#include "voxel_mesher.h"
#include <cstring>
#include <cstdint>
#include <cmath>

struct MaskCell {
    BlockType type;
    int sign;
};

// Forward declaration - defined in texturing file
void CalculateLightingAndTexture(BlockType blockType, Face face, int axis, int sign, int du, int dv,
                                  float& uBase, float& vBase, float& cellSize,
                                  uint8_t& cr, uint8_t& cg, uint8_t& cb, uint8_t& ca);

Mesh VoxelMesher::GenerateChunkMesh(const Chunk& chunk,
                                     const std::unordered_map<long long, Chunk>* neighborChunks) {
    std::vector<Vertex> vertices;
    vertices.reserve(CHUNK_SIZE * CHUNK_SIZE * 6);

    auto emitQuad = [&](int axis, int sign, int slice, int u0, int v0, int du, int dv, BlockType blockType) {
        float nx = 0, ny = 0, nz = 0;
        if (axis == 0) nx = (float)sign;
        if (axis == 1) ny = (float)sign;
        if (axis == 2) nz = (float)sign;

        Face face = FACE_FRONT;
        if (axis == 0) face = (sign > 0) ? FACE_RIGHT : FACE_LEFT;
        if (axis == 1) face = (sign > 0) ? FACE_TOP : FACE_BOTTOM;
        if (axis == 2) face = (sign > 0) ? FACE_FRONT : FACE_BACK;

        float uBase, vBase, cellSize;
        uint8_t cr, cg, cb, ca;
        CalculateLightingAndTexture(blockType, face, axis, sign, du, dv,
                                     uBase, vBase, cellSize, cr, cg, cb, ca);

        const float tu0 = uBase;
        const float tv0 = vBase;
        const float tu1 = uBase + cellSize * du;
        const float tv1 = vBase + cellSize * dv;

        auto push = [&](float px, float py, float pz, float uu, float vv) {
            vertices.push_back({px, py, pz, nx, ny, nz, uu, vv, cr, cg, cb, ca});
        };

        const float s = (float)slice;
        const float fu0 = (float)u0;
        const float fv0 = (float)v0;
        const float fu1 = (float)(u0 + du);
        const float fv1 = (float)(v0 + dv);

        float x00, y00, z00, x10, y10, z10, x01, y01, z01, x11, y11, z11;

        if (axis == 0) {
            x00 = s;  y00 = fu0; z00 = fv0;
            x10 = s;  y10 = fu1; z10 = fv0;
            x01 = s;  y01 = fu0; z01 = fv1;
            x11 = s;  y11 = fu1; z11 = fv1;
        } else if (axis == 1) {
            x00 = fv0; y00 = s;  z00 = fu0;
            x10 = fv1; y10 = s;  z10 = fu0;
            x01 = fv0; y01 = s;  z01 = fu1;
            x11 = fv1; y11 = s;  z11 = fu1;
        } else {
            x00 = fu0; y00 = fv0; z00 = s;
            x10 = fu1; y10 = fv0; z10 = s;
            x01 = fu0; y01 = fv1; z01 = s;
            x11 = fu1; y11 = fv1; z11 = s;
        }

        if (sign > 0) {
            push(x00, y00, z00, tu0, tv0);
            push(x10, y10, z10, tu1, tv0);
            push(x01, y01, z01, tu0, tv1);
            push(x01, y01, z01, tu0, tv1);
            push(x10, y10, z10, tu1, tv0);
            push(x11, y11, z11, tu1, tv1);
        } else {
            push(x00, y00, z00, tu0, tv0);
            push(x01, y01, z01, tu0, tv1);
            push(x10, y10, z10, tu1, tv0);
            push(x01, y01, z01, tu0, tv1);
            push(x11, y11, z11, tu1, tv1);
            push(x10, y10, z10, tu1, tv0);
        }
    };

    auto getBlockWithNeighbors = [&](int x, int y, int z) -> BlockType {
        if (chunk.isValid(x, y, z)) {
            return chunk.getBlock(x, y, z);
        }
        if (!neighborChunks) return BlockType::AIR;
        int ncx = chunk.cx, ncy = chunk.cy, ncz = chunk.cz;
        int lx = x, ly = y, lz = z;
        if (lx < 0) { ncx--; lx += CHUNK_SIZE; }
        else if (lx >= CHUNK_SIZE) { ncx++; lx -= CHUNK_SIZE; }
        if (ly < 0) { ncy--; ly += CHUNK_HEIGHT; }
        else if (ly >= CHUNK_HEIGHT) { ncy++; ly -= CHUNK_HEIGHT; }
        if (lz < 0) { ncz--; lz += CHUNK_SIZE; }
        else if (lz >= CHUNK_SIZE) { ncz++; lz -= CHUNK_SIZE; }
        auto it = neighborChunks->find(makeChunkKey(ncx, ncy, ncz));
        if (it == neighborChunks->end()) return BlockType::AIR;
        return it->second.getBlock(lx, ly, lz);
    };

    auto isFaceVisibleBetween = [&](BlockType solid, BlockType neighbor) -> bool {
        if (solid == BlockType::AIR) return false;
        if (!GetBlockProperties(solid).solid) return false;
        if (neighbor == BlockType::AIR) return true;
        return GetBlockProperties(solid).transparent != GetBlockProperties(neighbor).transparent;
    };

    for (int axis = 0; axis < 3; axis++) {
        int dim[3] = {CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE};
        int uAxis = (axis + 1) % 3;
        int vAxis = (axis + 2) % 3;
        int duDim = dim[uAxis];
        int dvDim = dim[vAxis];

        std::vector<MaskCell> mask((size_t)duDim * dvDim);

        for (int slice = 0; slice <= dim[axis]; slice++) {
            for (int v = 0; v < dvDim; v++) {
                for (int u = 0; u < duDim; u++) {
                    int ca[3] = {0,0,0}, cb[3] = {0,0,0};
                    ca[axis] = slice - 1; cb[axis] = slice;
                    ca[uAxis] = u; cb[uAxis] = u;
                    ca[vAxis] = v; cb[vAxis] = v;
                    BlockType a = getBlockWithNeighbors(ca[0], ca[1], ca[2]);
                    BlockType b = getBlockWithNeighbors(cb[0], cb[1], cb[2]);
                    MaskCell cell{BlockType::AIR, 0};
                    if (isFaceVisibleBetween(a, b)) { cell.type = a; cell.sign = +1; }
                    else if (isFaceVisibleBetween(b, a)) { cell.type = b; cell.sign = -1; }
                    mask[(size_t)u + (size_t)v * duDim] = cell;
                }
            }

            for (int v = 0; v < dvDim; v++) {
                for (int u = 0; u < duDim; ) {
                    MaskCell c = mask[(size_t)u + (size_t)v * duDim];
                    if (c.sign == 0 || c.type == BlockType::AIR) { u++; continue; }
                    int w = 1;
                    while (u + w < duDim) {
                        MaskCell cw = mask[(size_t)(u + w) + (size_t)v * duDim];
                        if (cw.sign != c.sign || cw.type != c.type) break;
                        w++;
                    }
                    int h = 1;
                    bool done = false;
                    while (v + h < dvDim && !done) {
                        for (int k = 0; k < w; k++) {
                            MaskCell ch = mask[(size_t)(u + k) + (size_t)(v + h) * duDim];
                            if (ch.sign != c.sign || ch.type != c.type) { done = true; break; }
                        }
                        if (!done) h++;
                    }
                    emitQuad(axis, c.sign, slice, u, v, w, h, c.type);
                    for (int hv = 0; hv < h; hv++) {
                        for (int hu = 0; hu < w; hu++) {
                            mask[(size_t)(u + hu) + (size_t)(v + hv) * duDim] = {BlockType::AIR, 0};
                        }
                    }
                    u += w;
                }
            }
        }
    }

    Mesh mesh = {0};
    if (vertices.empty()) return mesh;
    mesh.vertexCount = (int)vertices.size();
    mesh.triangleCount = mesh.vertexCount / 3;
    mesh.vertices = new float[mesh.vertexCount * 3];
    mesh.normals = new float[mesh.vertexCount * 3];
    mesh.texcoords = new float[mesh.vertexCount * 2];
    mesh.colors = new unsigned char[mesh.vertexCount * 4];
    for (int i = 0; i < mesh.vertexCount; i++) {
        mesh.vertices[i*3+0] = vertices[i].x;
        mesh.vertices[i*3+1] = vertices[i].y;
        mesh.vertices[i*3+2] = vertices[i].z;
        mesh.normals[i*3+0] = vertices[i].nx;
        mesh.normals[i*3+1] = vertices[i].ny;
        mesh.normals[i*3+2] = vertices[i].nz;
        mesh.texcoords[i*2+0] = vertices[i].u;
        mesh.texcoords[i*2+1] = vertices[i].v;
        mesh.colors[i*4+0] = vertices[i].cr;
        mesh.colors[i*4+1] = vertices[i].cg;
        mesh.colors[i*4+2] = vertices[i].cb;
        mesh.colors[i*4+3] = vertices[i].ca;
    }
    UploadMesh(&mesh, false);
    return mesh;
}
