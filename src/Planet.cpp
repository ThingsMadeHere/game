// src/Planet.cpp
#include "Planet.h"
#include <cstdio>
#include <ctime>

// Simple logger using C file operations
class Logger {
public:
    static void Log(const char* message) {
        FILE* logFile = fopen("terrain.log", "a");
        if (logFile) {
            time_t now = time(nullptr);
            char timeStr[100];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
            fprintf(logFile, "[%s] %s\n", timeStr, message);
            fclose(logFile);
        }
    }
    
    static void LogChunk(int chunkX, int chunkZ, int vertexCount, int triangleCount) {
        FILE* logFile = fopen("terrain.log", "a");
        if (logFile) {
            fprintf(logFile, "Chunk [%d,%d] - %d vertices, %d triangles\n", 
                   chunkX, chunkZ, vertexCount, triangleCount);
            fclose(logFile);
        }
    }
};

Planet::Planet(float planetRadius, int chunkResolution)
    : planetRadius(planetRadius), chunkResolution(chunkResolution)
{
    Logger::Log("Planet constructor started");
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
    Logger::Log("Planet constructor completed");
}

Planet::~Planet() {
    for (Chunk* chunk : chunks) {
        if (IsMeshReady(chunk->mesh)) UnloadMesh(chunk->mesh);
        delete chunk;
    }
    delete threadPool;
    UnloadRenderTexture(shadowMap);
}

// Simple 3D noise function (simplified Simplex)
float noise3D(float x, float y, float z) {
    // Simple implementation using sine waves (could replace with proper Simplex)
    float scale = 0.05f;
    return (sinf(x * scale) + sinf(y * scale * 0.5f) + sinf(z * scale)) * 0.333f;
}

float Planet::DensityFunction(float x, float y, float z) {
    // Much more aggressive terrain generation to ensure visible terrain
    float height = 0;
    
    // Large rolling hills - guaranteed to generate terrain
    height += sinf(x * 0.02f) * cosf(z * 0.02f) * 15.0f;
    
    // Medium features
    height += sinf(x * 0.05f + 1.0f) * cosf(z * 0.05f + 2.0f) * 8.0f;
    
    // Small details
    height += sinf(x * 0.1f) * cosf(z * 0.1f) * 3.0f;
    
    // Ensure minimum ground level
    height += 10.0f;
    
    return height - y;
}

bool Planet::IsMeshReady(const Mesh& mesh) const {
    return mesh.vertexCount > 0 && mesh.vertices != nullptr;
}

Mesh Planet::LoadMeshFromData(const MeshData& data) {
    Mesh mesh = {0};
    mesh.vertexCount = data.vertices.size();
    mesh.triangleCount = data.indices.size() / 3;

    // Safety check - don't process empty meshes
    if (mesh.vertexCount == 0 || data.indices.size() == 0) {
        return mesh;
    }

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

    // Indices - check for overflow
    mesh.triangleCount = data.indices.size() / 3;
    mesh.indices = new unsigned short[data.indices.size()];
    for (size_t i = 0; i < data.indices.size(); i++) {
        // Safety check - ensure index is within range
        if (data.indices[i] < mesh.vertexCount) {
            mesh.indices[i] = static_cast<unsigned short>(data.indices[i]);
        } else {
            mesh.indices[i] = 0; // Fallback to safe value
        }
    }

    UploadMesh(&mesh, false);
    return mesh;
}

void Planet::GenerateChunk(Chunk* chunk) {
    char msg[100];
    snprintf(msg, sizeof(msg), "Generating chunk at (%d,%d,%d)", chunk->chunkX, chunk->chunkY, chunk->chunkZ);
    Logger::Log(msg);
    
    // Use the class density function
    MeshData meshData = MarchingCubes::GenerateChunk(
        chunk->chunkX, chunk->chunkY, chunk->chunkZ,
        10.0f, chunkResolution,
        [this](float x, float y, float z) { return DensityFunction(x, y, z); }
    );

    if (IsMeshReady(chunk->mesh)) UnloadMesh(chunk->mesh);
    chunk->mesh = LoadMeshFromData(meshData);
    
    // Log the results
    Logger::LogChunk(chunk->chunkX, chunk->chunkZ, meshData.vertices.size(), meshData.indices.size() / 3);
    
    std::lock_guard<std::mutex> lock(chunk->mutex);
    chunk->isDirty = false;
    chunk->isLoading = false;
}

void Planet::Update(Vector3 playerPosition) {
    // Calculate which chunks should be loaded based on player position
    const int renderDistance = 2; // Load chunks within this distance
    const int chunkSize = 10; // Size of each chunk
    static int chunksGeneratedThisFrame = 0;
    const int maxChunksPerFrame = 1; // Rate limit chunk generation
    
    // Mark existing chunks for removal if too far
    for (auto it = chunks.begin(); it != chunks.end();) {
        Chunk* chunk = *it;
        float dx = chunk->chunkX - playerPosition.x;
        float dz = chunk->chunkZ - playerPosition.z;
        float distance = sqrtf(dx*dx + dz*dz);
        
        if (distance > renderDistance * chunkSize * 1.5f) { // Give more margin before unloading
            // Unload mesh and remove chunk
            if (IsMeshReady(chunk->mesh)) UnloadMesh(chunk->mesh);
            delete chunk;
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
    
    // Generate new chunks as needed (rate limited)
    if (chunksGeneratedThisFrame < maxChunksPerFrame) {
        // For now, generate a fixed grid around origin to test
        static int gridX = -2;
        static int gridZ = -2;
        
        if (gridX <= 2 && gridZ <= 2) {
            int chunkX = gridX * chunkSize;
            int chunkZ = gridZ * chunkSize;
            
            // Check if chunk already exists
            bool exists = false;
            for (Chunk* chunk : chunks) {
                if (chunk->chunkX == chunkX && chunk->chunkZ == chunkZ) {
                    exists = true;
                    break;
                }
            }
            
            // Create new chunk if it doesn't exist
            if (!exists) {
                Chunk* chunk = new Chunk();
                chunk->chunkX = chunkX;
                chunk->chunkY = 0;
                chunk->chunkZ = chunkZ;
                chunk->isDirty = true;
                chunks.push_back(chunk);
                
                // Generate synchronously for now
                GenerateChunk(chunk);
                chunksGeneratedThisFrame++;
                
                char msg[100];
                snprintf(msg, sizeof(msg), "Generated grid chunk at (%d,%d)", chunkX, chunkZ);
                Logger::Log(msg);
            }
            
            // Move to next grid position
            gridX++;
            if (gridX > 2) {
                gridX = -2;
                gridZ++;
            }
        }
    }
    
    // Reset counter periodically (simple approach)
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter >= 60) { // Reset every second
        chunksGeneratedThisFrame = 0;
        frameCounter = 0;
    }
}

void Planet::Draw(Shader shader) {
    // Get shader locations
    int modelLoc = GetShaderLocation(shader, "model");
    
    int chunksDrawn = 0;
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
            
            // Set shader uniform and draw
            SetShaderValueMatrix(shader, modelLoc, transform);
            DrawMesh(chunk->mesh, LoadMaterialDefault(), transform);
            chunksDrawn++;
        }
    }
    
    // Log rendering info periodically
    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter >= 300) { // Every 5 seconds at 60fps
        char msg[100];
        snprintf(msg, sizeof(msg), "Drew %d chunks, total chunks: %zu", chunksDrawn, chunks.size());
        Logger::Log(msg);
        frameCounter = 0;
    }
}
