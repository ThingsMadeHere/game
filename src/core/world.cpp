#include "world.h"
#include "planet.h"
#include "../rendering/voxel_mesher.h"
#include "../terrain/terrain.h"
#include "raylib.h"
#include "raymath.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>

// ============================================================================
// INFINITE WORLD SYSTEM
// ============================================================================
// This system manages infinite procedural terrain by:
// 1. Loading chunks around the player on-demand
// 2. Unloading distant chunks to manage memory
// 3. Using multi-layer noise for interesting terrain features

const int MAX_LOADED_CHUNKS = 512;      // Maximum chunks to keep in memory
const int CHUNK_LOAD_RADIUS = 8;         // Radius to load chunks around player
const int CHUNK_UNLOAD_RADIUS = 12;      // Radius beyond which chunks are unloaded
const int CHUNK_VERTICAL_RADIUS = 2;     // How many chunks to load above/below player

// Advanced terrain generation with multiple noise layers
struct TerrainLayer {
    float frequency;
    float amplitude;
    int octaves;
    float persistence;
    float lacunarity;
};

// Define terrain layers for varied landscape
static const TerrainLayer TERRAIN_LAYERS[] = {
    // Base terrain shape (large hills/valleys)
    {0.003f, 60.0f, 4, 0.5f, 2.0f},
    // Medium details (rolling terrain)
    {0.015f, 25.0f, 3, 0.5f, 2.0f},
    // Small details (roughness)
    {0.06f, 8.0f, 2, 0.5f, 2.0f},
    // Micro details (surface roughness)
    {0.15f, 3.0f, 2, 0.5f, 2.0f}
};
static const int NUM_TERRAIN_LAYERS = 4;

// 3D noise layers for caves and overhangs
static const TerrainLayer CAVE_LAYERS[] = {
    {0.03f, 1.0f, 3, 0.5f, 2.0f},
    {0.1f, 0.5f, 2, 0.5f, 2.0f}
};
static const int NUM_CAVE_LAYERS = 2;

// Evaluate multi-layer terrain noise at position
float GetTerrainHeightAdvanced(float worldX, float worldZ) {
    float height = 30.0f; // Base height
    
    // Sum all terrain layers
    for (int i = 0; i < NUM_TERRAIN_LAYERS; i++) {
        const TerrainLayer& layer = TERRAIN_LAYERS[i];
        float noise = FBM(worldX * layer.frequency, worldZ * layer.frequency, 0.0f, layer.octaves);
        height += noise * layer.amplitude;
    }
    
    // Add some ridged noise for dramatic peaks
    float ridgeNoise = 1.0f - fabsf(FBM(worldX * 0.015f, worldZ * 0.015f, 0.0f, 3));
    ridgeNoise = powf(ridgeNoise, 3.0f); // Sharpen ridges
    height += ridgeNoise * 25.0f;
    
    // Ensure minimum height for playable area
    if (height < 5.0f) height = 5.0f;
    
    return height;
}

// 3D noise for caves
float GetCaveDensity(float worldX, float worldY, float worldZ) {
    float density = 0.0f;
    for (int i = 0; i < NUM_CAVE_LAYERS; i++) {
        const TerrainLayer& layer = CAVE_LAYERS[i];
        density += FBM(worldX * layer.frequency, worldY * layer.frequency * 0.5f, 
                        worldZ * layer.frequency, layer.octaves) * layer.amplitude;
    }
    return density;
}

// Get block type based on depth and terrain type
BlockType GetBlockTypeForTerrain(float worldY, float terrainHeight, float worldX, float worldZ) {
    float depth = terrainHeight - worldY;
    
    // Surface layer
    if (depth < 1.0f) {
        // Beach/sand near water level (water at height 18)
        if (terrainHeight < 22.0f) {
            return BlockType::SAND;
        }
        // Snow on high peaks
        if (terrainHeight > 80.0f) {
            return BlockType::SNOW;
        }
        // Grass for normal terrain
        return BlockType::GRASS;
    }
    // Dirt layer
    else if (depth < 4.0f) {
        return BlockType::DIRT;
    }
    // Stone with occasional ore
    else {
        // Random ore generation at deeper levels
        if (depth > 15.0f) {
            float oreNoise = SmoothNoise3D(worldX * 0.1f, worldY * 0.1f, worldZ * 0.1f);
            if (oreNoise > 0.7f) return BlockType::METAL;
        }
        return BlockType::STONE;
    }
}

// ============================================================================
bool IsChunkVisible(const Camera3D& camera, int cx, int cy, int cz) {
    // Chunk center in WORLD coordinates (accounting for VOXEL_SIZE)
    Vector3 chunkCenter = {
        (float)(cx * CHUNK_SIZE + CHUNK_SIZE / 2.0f) * VOXEL_SIZE,
        (float)(cy * CHUNK_HEIGHT + CHUNK_HEIGHT / 2.0f) * VOXEL_SIZE,
        (float)(cz * CHUNK_SIZE + CHUNK_SIZE / 2.0f) * VOXEL_SIZE
    };
    
    Vector3 diff = {
        camera.position.x - chunkCenter.x,
        camera.position.y - chunkCenter.y,
        camera.position.z - chunkCenter.z
    };
    
    float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    // Use CHUNK_HEIGHT for calculation since chunks are tall (64 blocks)
    // and we need to see chunks above/below player
    return distance < (RENDER_DISTANCE * CHUNK_HEIGHT * VOXEL_SIZE * 1.5f);
}

// Chunk management for infinite terrain with vertical stacking
void World::UpdateChunkLoading(const Vector3& playerPos) {
    int playerChunkX = (int)floorf(playerPos.x / (CHUNK_SIZE * VOXEL_SIZE));
    int playerChunkY = (int)floorf(playerPos.y / (CHUNK_HEIGHT * VOXEL_SIZE));
    int playerChunkZ = (int)floorf(playerPos.z / (CHUNK_SIZE * VOXEL_SIZE));
    
    // Queue chunks to load around player (horizontal + vertical)
    for (int cx = playerChunkX - CHUNK_LOAD_RADIUS; cx <= playerChunkX + CHUNK_LOAD_RADIUS; cx++) {
        for (int cz = playerChunkZ - CHUNK_LOAD_RADIUS; cz <= playerChunkZ + CHUNK_LOAD_RADIUS; cz++) {
            // Check horizontal distance
            int distX = abs(cx - playerChunkX);
            int distZ = abs(cz - playerChunkZ);
            int horizontalDist = (int)sqrtf(distX * distX + distZ * distZ);
            
            if (horizontalDist <= CHUNK_LOAD_RADIUS) {
                // Sample terrain height at center of this column
                float worldX = (float)(cx * CHUNK_SIZE + CHUNK_SIZE / 2) * VOXEL_SIZE;
                float worldZ = (float)(cz * CHUNK_SIZE + CHUNK_SIZE / 2) * VOXEL_SIZE;
                float terrainHeight = GetTerrainHeightAdvanced(worldX, worldZ);
                int terrainChunkY = (int)(terrainHeight / (CHUNK_HEIGHT * VOXEL_SIZE));
                
                // Load chunks from bottom up to terrain surface + a buffer above
                int minChunkY = 0;
                int maxChunkY = terrainChunkY + 1; // +1 to include surface chunks
                
                // Also load chunks around player height
                int playerMinY = playerChunkY - CHUNK_VERTICAL_RADIUS;
                int playerMaxY = playerChunkY + CHUNK_VERTICAL_RADIUS;
                
                // Combine both ranges
                int loadMinY = 0;  // Always start from bottom
                int loadMaxY = fmaxf(maxChunkY, playerMaxY);
                
                // Load vertical column of chunks at this X,Z position
                for (int cy = loadMinY; cy <= loadMaxY; cy++) {
                    // Check vertical distance from player
                    int distY = abs(cy - playerChunkY);
                    int totalDist = (int)sqrtf(distX*distX + distY*distY + distZ*distZ);
                    
                    if (totalDist <= CHUNK_LOAD_RADIUS + CHUNK_VERTICAL_RADIUS) {
                        long long key = makeChunkKey(cx, cy, cz);
                        if (chunks.find(key) == chunks.end()) {
                            // Chunk doesn't exist, queue for generation
                            QueueChunkGeneration(cx, cy, cz);
                        }
                    }
                }
            }
        }
    }
    
    // Unload distant chunks (3D distance check)
    std::vector<long long> chunksToUnload;
    for (auto& [key, chunk] : chunks) {
        int distX = abs(chunk.cx - playerChunkX);
        int distY = abs(chunk.cy - playerChunkY);
        int distZ = abs(chunk.cz - playerChunkZ);
        int dist = (int)sqrtf(distX * distX + distY * distY + distZ * distZ);
        
        if (dist > CHUNK_UNLOAD_RADIUS) {
            chunksToUnload.push_back(key);
        }
    }
    
    // Actually unload the chunks
    for (long long key : chunksToUnload) {
        auto it = chunks.find(key);
        if (it != chunks.end()) {
            // Unload mesh if it exists
            if (it->second.meshGenerated && it->second.mesh.vertexCount > 0) {
                UnloadMesh(it->second.mesh);
            }
            chunks.erase(it);
        }
    }
    
    // Debug output
    static int lastChunkCount = 0;
    if ((int)chunks.size() != lastChunkCount) {
        printf("Loaded chunks: %zu (unloaded %zu distant chunks)\n", 
               chunks.size(), chunksToUnload.size());
        lastChunkCount = (int)chunks.size();
    }
}


void ChunkWorker(World* world) {
    while (world->IsRunning()) {
        ChunkRequest req = world->PopChunkRequest();
        if (req.cx == -999) {
            break; // Shutdown signal
        }
        
        // Generate chunk terrain using the same advanced generation as main thread
        long long key = makeChunkKey(req.cx, req.cy, req.cz);
        Chunk chunk(req.cx, req.cy, req.cz);
        
        // Use the same GenerateTerrain function for consistency
        // We need to generate manually here since we're in a worker thread
        int blocksGenerated = 0;
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                float worldX = (float)(chunk.cx * CHUNK_SIZE + x) * VOXEL_SIZE;
                float worldZ = (float)(chunk.cz * CHUNK_SIZE + z) * VOXEL_SIZE;
                
                // Use advanced multi-layer noise (same as GenerateTerrain)
                float terrainHeight = GetTerrainHeightAdvanced(worldX, worldZ);
                
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    float worldY = (float)(chunk.cy * CHUNK_HEIGHT + y) * VOXEL_SIZE;
                    
                    if (worldY < terrainHeight) {
                        // Check for caves
                        float caveDensity = GetCaveDensity(worldX, worldY, worldZ);
                        
                        if (caveDensity < 0.3f || worldY > terrainHeight - 5.0f) {
                            BlockType blockType = GetBlockTypeForTerrain(worldY, terrainHeight, worldX, worldZ);
                            chunk.setVoxel(x, y, z, blockType);
                            blocksGenerated++;
                        }
                    }
                }
            }
        }
        
        // Generate mesh from voxel data
        chunk.mesh = VoxelMesher::GenerateChunkMesh(chunk, nullptr);
        if (chunk.mesh.vertexCount > 0) {
            chunk.meshGenerated = true;
        } else {
            chunk.meshGenerated = false;
        }
        chunk.needsUpdate = false;
        
        // Debug for problematic chunks
        if ((chunk.cx == 0 && chunk.cy == 0 && chunk.cz == 1) || 
            (chunk.cx == 1 && chunk.cy == 0 && chunk.cz == 1)) {
            printf("WORKER: Chunk (%d,%d,%d) generated with %d blocks, %d vertices\n", 
                   chunk.cx, chunk.cy, chunk.cz, blocksGenerated, chunk.mesh.vertexCount);
        }
        
        // Add to world
        std::lock_guard<std::mutex> lock(world->GetMutex());
        auto result = world->GetChunks().emplace(key, chunk);
        if (result.second) {
            printf("Worker: Generated chunk (%d,%d,%d) with %d blocks, %d vertices\n", 
                   req.cx, req.cy, req.cz, blocksGenerated, chunk.mesh.vertexCount);
        }
    }
}

// Helper function to get chunk without creating new ones
Chunk* World::GetChunkIfExists(int cx, int cy, int cz) {
    long long key = makeChunkKey(cx, cy, cz);
    auto it = chunks.find(key);
    return (it != chunks.end()) ? &it->second : nullptr;
}

Chunk& World::GetChunk(int cx, int cy, int cz) {
    long long key = makeChunkKey(cx, cy, cz);
    auto it = chunks.find(key);
    if (it == chunks.end()) {
        Chunk chunk(cx, cy, cz);
        GenerateTerrain(chunk);
        auto result = chunks.emplace(key, chunk);
        return result.first->second;
    }
    return it->second;
}

void World::GenerateTerrain(Chunk& chunk) {
    printf("Generating terrain for chunk (%d, %d, %d)\n", chunk.cx, chunk.cy, chunk.cz);
    
    int blocksGenerated = 0;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            float worldX = (float)(chunk.cx * CHUNK_SIZE + x) * VOXEL_SIZE;
            float worldZ = (float)(chunk.cz * CHUNK_SIZE + z) * VOXEL_SIZE;
            
            // Get terrain height using advanced multi-layer noise
            float terrainHeight = GetTerrainHeightAdvanced(worldX, worldZ);
            
            // Generate column from bottom up
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                float worldY = (float)(chunk.cy * CHUNK_HEIGHT + y) * VOXEL_SIZE;
                
                // Check if this position is below terrain surface
                if (worldY < terrainHeight) {
                    // Check for caves using 3D noise
                    float caveDensity = GetCaveDensity(worldX, worldY, worldZ);
                    
                    // Only place block if not in a cave (cave threshold)
                    if (caveDensity < 0.3f || worldY > terrainHeight - 5.0f) {
                        BlockType blockType = GetBlockTypeForTerrain(worldY, terrainHeight, worldX, worldZ);
                        chunk.setVoxel(x, y, z, blockType);
                        blocksGenerated++;
                    }
                }
            }
        }
    }
    
    printf("Generated %d blocks in chunk (%d, %d, %d)\n", blocksGenerated, chunk.cx, chunk.cy, chunk.cz);
    
    // Generate mesh using voxel mesher
    if (chunk.mesh.vertexCount > 0) {
        UnloadMesh(chunk.mesh);
    }
    chunk.mesh = VoxelMesher::GenerateChunkMesh(chunk, &chunks);
    if (chunk.mesh.vertexCount > 0) {
        chunk.meshGenerated = true;
    } else {
        chunk.meshGenerated = false;
    }
    chunk.needsUpdate = false;
    
    printf("Generated mesh with %d vertices for chunk (%d, %d, %d)\n", chunk.mesh.vertexCount, chunk.cx, chunk.cy, chunk.cz);
}

void World::UpdateChunk(int cx, int cy, int cz) {
    long long key = makeChunkKey(cx, cy, cz);
    auto it = chunks.find(key);
    if (it != chunks.end()) {
        if (it->second.needsUpdate) {
            printf("Updating dirty chunk (%d,%d,%d)\n", cx, cy, cz);
            
            // Regenerate mesh if needed
            if (!it->second.meshGenerated || it->second.needsUpdate) {
                if (it->second.mesh.vertexCount > 0) {
                    UnloadMesh(it->second.mesh);
                }
                it->second.mesh = VoxelMesher::GenerateChunkMesh(it->second, &chunks);
                if (it->second.mesh.vertexCount > 0) {
                    it->second.meshGenerated = true;
                } else {
                    it->second.meshGenerated = false;
                }
                it->second.needsUpdate = false;
            }
        } else {
            // Debug: Show that chunk is clean and not being updated
            static int debugCounter = 0;
            if (debugCounter++ % 300 == 0) { // Print every 300 frames to avoid spam
                printf("Chunk (%d,%d,%d) is clean, skipping update\n", cx, cy, cz);
            }
        }
    }
}

void World::SetVoxel(int cx, int cy, int cz, int x, int y, int z, BlockType type) {
    Chunk& chunk = GetChunk(cx, cy, cz);
    chunk.setVoxel(x, y, z, type);
    chunk.needsUpdate = true;
}

BlockType World::GetVoxel(int cx, int cy, int cz, int x, int y, int z) {
    Chunk* chunk = GetChunkIfExists(cx, cy, cz);
    if (chunk == nullptr) return BlockType::AIR; // Empty/air if chunk doesn't exist
    return chunk->getBlock(x, y, z);
}

void World::Init() {
    running = true;
    
    // Initialize noise presets first
    InitPlanetNoisePresets();
    
    // Load all planet configurations
    if (!g_PlanetSystem.LoadAllPlanetsFromDirectory("../data/planets/")) {
        printf("WARNING: Failed to load some planet configs\n");
    }
    
    // Set active planet to Etaui (the hellscape starting world)
    g_PlanetSystem.SetActivePlanet("etaui");
    printf("Active planet set to: Etaui\n");
    
    // Start worker threads for chunk generation
    int numThreads = std::thread::hardware_concurrency();
    if (numThreads < 2) numThreads = 2;
    if (numThreads > 8) numThreads = 8; // Cap at 8 threads
    
    for (int i = 0; i < numThreads; i++) {
        workerThreads.emplace_back(ChunkWorker, this);
    }
    
    printf("Started %d worker threads for chunk generation\n", numThreads);
}

void World::Cleanup() {
    // Stop worker threads
    running = false;
    
    // Wake up all worker threads
    for (size_t i = 0; i < workerThreads.size(); i++) {
        QueueChunkGeneration(-999, 0, 0); // Shutdown signal
    }
    
    // Wait for all threads to finish
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Unload all chunk meshes to prevent segfault
    for (auto& [key, chunk] : chunks) {
        if (chunk.meshGenerated && chunk.mesh.vertexCount > 0) {
            UnloadMesh(chunk.mesh);
            chunk.meshGenerated = false;
        }
    }
    chunks.clear();
}

void World::SaveWorld(const std::string& filename) {
    printf("DEBUG: Saving world to %s\n", filename.c_str());
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        printf("ERROR: Could not open file for writing: %s\n", filename.c_str());
        return;
    }
    
    // Write world seed (using current time as simple seed)
    auto now = std::chrono::system_clock::now();
    uint64_t seed = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    file.write(reinterpret_cast<const char*>(&seed), sizeof(seed));
    
    // Write modified chunk count
    int modifiedCount = GetModifiedChunkCount();
    file.write(reinterpret_cast<const char*>(&modifiedCount), sizeof(modifiedCount));
    
    // Write each modified chunk
    for (const auto& [key, chunk] : chunks) {
        if (chunk.needsUpdate || chunk.meshGenerated) {
            // Write chunk key (encoded as long long)
            int cx = (int)(key >> 32);
            int cy = (int)((key >> 16) & 0xFFFF);
            int cz = (int)(key & 0xFFFF);
            file.write(reinterpret_cast<const char*>(&cx), sizeof(cx));
            file.write(reinterpret_cast<const char*>(&cy), sizeof(cy));
            file.write(reinterpret_cast<const char*>(&cz), sizeof(cz));
            
            // Write chunk voxel data (block types)
            for (int i = 0; i < CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE; i++) {
                unsigned char blockType = (unsigned char)chunk.voxels[i].type;
                file.write(reinterpret_cast<const char*>(&blockType), sizeof(blockType));
            }
            
            // Write chunk state
            file.write(reinterpret_cast<const char*>(&chunk.meshGenerated), sizeof(chunk.meshGenerated));
            file.write(reinterpret_cast<const char*>(&chunk.needsUpdate), sizeof(chunk.needsUpdate));
        }
    }
    
    file.close();
    printf("DEBUG: Saved %d modified chunks\n", modifiedCount);
}

int World::GetModifiedChunkCount() {
    int count = 0;
    for (const auto& [key, chunk] : chunks) {
        if (chunk.needsUpdate || chunk.meshGenerated) {
            count++;
        }
    }
    return count;
}

void World::Render(const Camera3D& camera) {
    int chunksRendered = 0;
    long long totalVertices = 0;
    
    // Calculate player position for distance-based culling
    Vector3 playerPos = camera.position;
    int playerChunkX = (int)floorf(playerPos.x / (CHUNK_SIZE * VOXEL_SIZE));
    int playerChunkY = (int)floorf(playerPos.y / (CHUNK_HEIGHT * VOXEL_SIZE));
    int playerChunkZ = (int)floorf(playerPos.z / (CHUNK_SIZE * VOXEL_SIZE));
    
    for (auto& [key, chunk] : chunks) {
        // Frustum-ish distance check (IsChunkVisible handles the actual culling)
        if (!IsChunkVisible(camera, chunk.cx, chunk.cy, chunk.cz)) {
            continue;
        }

        if (chunk.meshGenerated && chunk.mesh.vertexCount > 0) {
            Material mat = LoadMaterialDefault();
            Vector3 chunkPos = {
                (float)(chunk.cx * CHUNK_SIZE) * VOXEL_SIZE,
                (float)(chunk.cy * CHUNK_HEIGHT) * VOXEL_SIZE,
                (float)(chunk.cz * CHUNK_SIZE) * VOXEL_SIZE
            };
            Matrix model = MatrixTranslate(chunkPos.x, chunkPos.y, chunkPos.z);
            DrawMesh(chunk.mesh, mat, model);
            chunksRendered++;
            totalVertices += chunk.mesh.vertexCount;
            
            // Log visible verts for this chunk
            printf("Chunk (%d,%d,%d): %d vertices\n", 
                   chunk.cx, chunk.cy, chunk.cz, chunk.mesh.vertexCount);
        } else {
            // Debug: Check if it's one of the problematic chunks
            if ((chunk.cx == 0 && chunk.cy == 0 && chunk.cz == 1) || 
                (chunk.cx == 1 && chunk.cy == 0 && chunk.cz == 1)) {
                printf("DEBUG: Chunk (%d,%d,%d) not rendered - meshGenerated=%d, vertexCount=%d\n", 
                       chunk.cx, chunk.cy, chunk.cz, chunk.meshGenerated, chunk.mesh.vertexCount);
            }
        }
    }
    
    if (chunksRendered > 0) {
        printf("Drew %d chunks, %lld total vertices, avg %lld per chunk\n", 
               chunksRendered, totalVertices, totalVertices / chunksRendered);
    }
    
    // Debug: Print player position and nearby chunks
    printf("Player pos: (%.1f, %.1f, %.1f), Player chunk: (%d, %d, %d)\n", 
           playerPos.x, playerPos.y, playerPos.z, playerChunkX, playerChunkY, playerChunkZ);
    
    // Draw all models (separate from voxel terrain)
    modelManager.DrawAll();
}

void World::RenderShadows(const Matrix& lightSpaceMatrix, Shader& shadowShader) {
    // Shadow rendering not needed with voxel mesher approach
    // Chunks are rendered directly with DrawMesh
}

void World::AddModel(const std::string& name, const std::string& objPath,
                     const std::string& texPath, Vector3 position,
                     Vector3 rotation, float scale) {
    modelManager.AddModel(name, objPath, texPath, position, rotation, scale);
}

void World::DrawModels() {
    modelManager.DrawAll();
}

void World::QueueChunkGeneration(int cx, int cy, int cz) {
    std::lock_guard<std::mutex> lock(queueMutex);
    chunkQueue.push({cx, cy, cz});
}

ChunkRequest World::PopChunkRequest() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (!chunkQueue.empty()) {
        ChunkRequest req = chunkQueue.front();
        chunkQueue.pop();
        return req;
    }
    return {-999, -999, -999}; // Empty signal
}

int World::GetGeneratedChunkCount() const {
    return chunks.size();
}

void World::GenerateMesh(Chunk& chunk) {
    // Unload old mesh if it exists
    if (chunk.mesh.vertexCount > 0) {
        UnloadMesh(chunk.mesh);
    }
    
    chunk.mesh = VoxelMesher::GenerateChunkMesh(chunk, &chunks);
    if (chunk.mesh.vertexCount > 0) {
        chunk.meshGenerated = true;
    } else {
        chunk.meshGenerated = false;
    }
    chunk.needsUpdate = false;
    
    // Regenerate neighbor chunks to ensure seamless boundaries
    RegenerateNeighborChunks(chunk.cx, chunk.cy, chunk.cz);
}

void World::GenerateAllMeshes() {
    // Generate meshes for all chunks after terrain is loaded
    for (auto& [key, chunk] : chunks) {
        if (!chunk.meshGenerated) {
            GenerateMesh(chunk);
        }
    }
}

void World::RegenerateNeighborChunks(int cx, int cy, int cz) {
    // Regenerate all neighboring chunks to ensure seamless boundaries
    int neighbors[6][3] = {
        {cx-1, cy, cz}, {cx+1, cy, cz},  // X neighbors
        {cx, cy-1, cz}, {cx, cy+1, cz},  // Y neighbors  
        {cx, cy, cz-1}, {cx, cy, cz+1}   // Z neighbors
    };
    
    for (int i = 0; i < 6; i++) {
        int nx = neighbors[i][0];
        int ny = neighbors[i][1];
        int nz = neighbors[i][2];
        
        long long neighborKey = makeChunkKey(nx, ny, nz);
        auto it = chunks.find(neighborKey);
        if (it != chunks.end() && it->second.meshGenerated) {
            // Regenerate this neighbor's mesh with updated neighbor information
            it->second.mesh = VoxelMesher::GenerateChunkMesh(it->second, &chunks);
        }
    }
}

void World::ProcessChunkQueue() {
    // Process any pending chunks from queue (limit per frame to prevent stuttering)
    int chunksProcessedThisFrame = 0;
    const int MAX_CHUNKS_PER_FRAME = 1; // Very conservative to prevent stuttering
    const double MAX_PROCESSING_TIME_MS = 8.0; // Max 8ms per frame for chunk processing
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    while (chunksProcessedThisFrame < MAX_CHUNKS_PER_FRAME) {
        ChunkRequest req = PopChunkRequest();
        if (req.cx == -999) break;
        
        // Check if chunk already exists
        long long key = makeChunkKey(req.cx, req.cy, req.cz);
        if (chunks.find(key) != chunks.end()) {
            chunksProcessedThisFrame++;
            continue;
        }
        
        // Generate chunk
        Chunk chunk(req.cx, req.cy, req.cz);
        GenerateTerrain(chunk);
        
        // Add to world
        auto result = chunks.emplace(key, chunk);
        if (result.second) {
            result.first->second.mesh = VoxelMesher::GenerateChunkMesh(result.first->second, &chunks);
            if (result.first->second.mesh.vertexCount > 0) {
                result.first->second.meshGenerated = true;
            } else {
                result.first->second.meshGenerated = false;
            }
            result.first->second.needsUpdate = false;
        }
        
        chunksProcessedThisFrame++;
        
        // Check if we've exceeded our time budget
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
        if (elapsed.count() > MAX_PROCESSING_TIME_MS * 1000) {
            break;
        }
    }
}
