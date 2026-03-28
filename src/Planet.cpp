// src/Planet.cpp
#include "Planet.h"

Planet::Planet(float planetRadius, int chunkResolution)
    : planetRadius(planetRadius), chunkResolution(chunkResolution)
{
    threadPool = new ThreadPool(3);
    shadowMap = LoadRenderTexture(SHADOW_RES, SHADOW_RES);

    // Generate initial chunks synchronously for now
    for (int x = -1; x <= 1; x += 2) {
        for (int z = -1; z <= 1; z += 2) {
            Chunk* chunk = new Chunk();
            chunk->chunkX = x * 10;
            chunk->chunkY = 0;
            chunk->chunkZ = z * 10;
            chunk->isDirty = true;
            chunks.push_back(chunk);
            
            // Generate mesh synchronously to avoid threading issues with OpenGL
            GenerateChunk(chunk);
        }
    }
}

Planet::~Planet() {
    for (Chunk* chunk : chunks) {
        if (IsMeshReady(chunk->mesh)) UnloadMesh(chunk->mesh);
        delete chunk;
    }
    delete threadPool;
    UnloadRenderTexture(shadowMap);
}

float Planet::DensityFunction(float x, float y, float z) {
    // Simple sphere density for testing
    float dist = sqrt(x*x + y*y + z*z);
    return 50.0f - dist; // Planet radius of 50
}

bool Planet::IsMeshReady(const Mesh& mesh) const {
    return mesh.vboId != 0;
}

Mesh Planet::LoadMeshFromData(const MeshData& data) {
    Mesh mesh = {0};
    mesh.vertexCount = data.vertices.size();
    mesh.triangleCount = data.indices.size() / 3;

    // Vertex positions
    mesh.vertices = new float[mesh.vertexCount * 3];
    for (size_t i = 0; i < data.vertices.size(); i++) {
        mesh.vertices[i*3+0] = data.vertices[i].x;
        mesh.vertices[i*3+1] = data.vertices[i].y;
        mesh.vertices[i*3+2] = data.vertices[i].z;
    }

    // Normals
    mesh.normals = new float[mesh.vertexCount * 3];
    for (size_t i = 0; i < data.vertices.size(); i++) {
        mesh.normals[i*3+0] = data.vertices[i].nx;
        mesh.normals[i*3+1] = data.vertices[i].ny;
        mesh.normals[i*3+2] = data.vertices[i].nz;
    }

    // Colors
    mesh.colors = new unsigned char[mesh.vertexCount * 4];
    for (size_t i = 0; i < data.vertices.size(); i++) {
        mesh.colors[i*4+0] = data.vertices[i].r * 255;
        mesh.colors[i*4+1] = data.vertices[i].g * 255;
        mesh.colors[i*4+2] = data.vertices[i].b * 255;
        mesh.colors[i*4+3] = 255;
    }

    // Indices
    mesh.triangleCount = data.indices.size() / 3;
    mesh.indices = new unsigned short[data.indices.size()];
    for (size_t i = 0; i < data.indices.size(); i++) {
        mesh.indices[i] = static_cast<unsigned short>(data.indices[i]);
    }

    UploadMesh(&mesh, false);
    return mesh;
}

void Planet::GenerateChunk(Chunk* chunk) {
    MeshData meshData = MarchingCubes::GenerateChunk(
        chunk->chunkX, chunk->chunkY, chunk->chunkZ,
        10.0f, chunkResolution,
        [this](float x, float y, float z) { return DensityFunction(x, y, z); }
    );

    if (IsMeshReady(chunk->mesh)) UnloadMesh(chunk->mesh);
    
    chunk->mesh = LoadMeshFromData(meshData);
    
    std::lock_guard<std::mutex> lock(chunk->mutex);
    chunk->isDirty = false;
    chunk->isLoading = false;
}

void Planet::Update(Vector3 playerPosition) {
    // Disabled threading for now to avoid OpenGL threading issues
    /*
    for (Chunk* chunk : chunks) {
        if (chunk->isDirty && !chunk->isLoading) {
            chunk->isLoading = true;
            threadPool->enqueue([this, chunk]() { GenerateChunk(chunk); });
        }
    }
    */
}

void Planet::Draw(Shader shader, Matrix lightSpaceMatrix) {
    // Get shader locations
    int mvpLoc = GetShaderLocation(shader, "mvp");
    int modelLoc = GetShaderLocation(shader, "model");
    int lightSpaceLoc = GetShaderLocation(shader, "lightSpaceMatrix");
    
    for (Chunk* chunk : chunks) {
        if (IsMeshReady(chunk->mesh)) {
            // Create transform matrix for this chunk
            Matrix transform = { 0 };
            transform.m0 = 1.0f;
            transform.m5 = 1.0f;
            transform.m10 = 1.0f;
            transform.m12 = chunk->chunkX;
            transform.m13 = chunk->chunkY;
            transform.m14 = chunk->chunkZ;
            transform.m15 = 1.0f;
            
            // Set shader uniforms
            SetShaderValueMatrix(shader, modelLoc, transform);
            SetShaderValueMatrix(shader, lightSpaceLoc, lightSpaceMatrix);
            
            // Draw mesh with shader
            BeginShaderMode(shader);
                DrawMesh(chunk->mesh, LoadMaterialDefault(), transform);
            EndShaderMode();
        }
    }
}
