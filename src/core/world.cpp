#include "world.h"
#include "planet.h"
#include "../rendering/voxel_mesher.h"
#include "../terrain/terrain.h"
#include "raylib.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>

// Simple frustum culling check
bool IsChunkVisible(const Camera3D& camera, int cx, int cy, int cz) {
    Vector3 chunkCenter = {
        (float)(cx * CHUNK_SIZE + CHUNK_SIZE / 2.0f),
        (float)(cy * CHUNK_HEIGHT + CHUNK_HEIGHT / 2.0f),
        (float)(cz * CHUNK_SIZE + CHUNK_SIZE / 2.0f)
    };
    
    Vector3 diff = {
        camera.position.x - chunkCenter.x,
        camera.position.y - chunkCenter.y,
        camera.position.z - chunkCenter.z
    };
    
    float distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    return distance < (RENDER_DISTANCE * CHUNK_SIZE * 1.5f);
}

// Thread worker function for async chunk generation
void ChunkWorker(World* world) {
    while (world->IsRunning()) {
        ChunkRequest req = world->PopChunkRequest();
        if (req.cx == -999) {
            break; // Shutdown signal
        }
        
        // Generate chunk terrain
        long long key = makeChunkKey(req.cx, req.cy, req.cz);
        Chunk chunk(req.cx, req.cy, req.cz);
        
        // Get active planet for terrain generation
        PlanetDefinition* activePlanet = g_PlanetSystem.GetActivePlanet();
        std::string planetId = activePlanet ? activePlanet->id : "default";
        
        // Generate voxel data using noise
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    float worldX = (float)(chunk.cx * CHUNK_SIZE + x);
                    float worldY = (float)(chunk.cy * CHUNK_HEIGHT + y);
                    float worldZ = (float)(chunk.cz * CHUNK_SIZE + z);
                    
                    // Simple terrain height using noise
                    float baseHeight = 20.0f;
                    float noiseVal = FBM(worldX * 0.05f, worldZ * 0.05f, 0, 4);
                    float terrainHeight = baseHeight + noiseVal * 15.0f;
                    
                    if (worldY < terrainHeight - 5) {
                        chunk.setVoxel(x, y, z, BlockType::STONE);
                    } else if (worldY < terrainHeight - 1) {
                        chunk.setVoxel(x, y, z, BlockType::DIRT);
                    } else if (worldY < terrainHeight) {
                        // Surface block - grass or sand based on height
                        if (terrainHeight < 18.0f) {
                            chunk.setVoxel(x, y, z, BlockType::SAND);
                        } else {
                            chunk.setVoxel(x, y, z, BlockType::GRASS);
                        }
                    }
                    // Above terrain = air (already initialized)
                }
            }
        }
        
        // Generate mesh from voxel data
        if (chunk.meshGenerated && chunk.mesh.vertexCount > 0) {
            UnloadMesh(chunk.mesh);
        }
        chunk.mesh = VoxelMesher::GenerateChunkMesh(chunk, nullptr);
        chunk.meshGenerated = true;
        chunk.needsUpdate = false;
        
        // Add to world
        std::lock_guard<std::mutex> lock(world->GetMutex());
        auto result = world->GetChunks().emplace(key, chunk);
        if (result.second) {
            // Successfully added
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
    
    // Get active planet ID from planet system
    PlanetDefinition* activePlanet = g_PlanetSystem.GetActivePlanet();
    std::string planetId = activePlanet ? activePlanet->id : "etaui";
    
    int blocksGenerated = 0;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                float worldX = (float)(chunk.cx * CHUNK_SIZE + x) * VOXEL_SIZE;
                float worldY = (float)(chunk.cy * CHUNK_HEIGHT + y) * VOXEL_SIZE;
                float worldZ = (float)(chunk.cz * CHUNK_SIZE + z) * VOXEL_SIZE;
                
                // Simple terrain height using noise
                float baseHeight = 20.0f;
                float noiseVal = FBM(worldX * 0.05f, worldZ * 0.05f, 0, 4);
                float terrainHeight = baseHeight + noiseVal * 15.0f;
                
                // Debug: Print terrain height for first few blocks (DISABLED FOR PERFORMANCE)
                // static int debugCount = 0;
                // if (debugCount < 10) {
                //     printf("Terrain at (%.1f, %.1f): height=%.1f, worldY=%.1f\n", 
                //            worldX, worldZ, terrainHeight, worldY);
                //     debugCount++;
                // }
                
                if (worldY < terrainHeight - 5) {
                    chunk.setVoxel(x, y, z, BlockType::STONE);
                    blocksGenerated++;
                } else if (worldY < terrainHeight - 1) {
                    chunk.setVoxel(x, y, z, BlockType::DIRT);
                    blocksGenerated++;
                } else if (worldY < terrainHeight) {
                    if (terrainHeight < 18.0f) {
                        chunk.setVoxel(x, y, z, BlockType::SAND);
                    } else {
                        chunk.setVoxel(x, y, z, BlockType::GRASS);
                    }
                    blocksGenerated++;
                }
            }
        }
    }
    
    printf("Generated %d blocks in chunk (%d, %d, %d)\n", blocksGenerated, chunk.cx, chunk.cy, chunk.cz);
    
    // Generate mesh using voxel mesher
    if (chunk.meshGenerated && chunk.mesh.vertexCount > 0) {
        UnloadMesh(chunk.mesh);
    }
    chunk.mesh = VoxelMesher::GenerateChunkMesh(chunk, &chunks);
    chunk.meshGenerated = true;
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
                if (it->second.meshGenerated && it->second.mesh.vertexCount > 0) {
                    UnloadMesh(it->second.mesh);
                }
                it->second.mesh = VoxelMesher::GenerateChunkMesh(it->second, &chunks);
                it->second.meshGenerated = true;
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
    
    // Calculate player position for distance-based culling
    Vector3 playerPos = camera.position;
    int playerChunkX = (int)floorf(playerPos.x / (CHUNK_SIZE * VOXEL_SIZE));
    int playerChunkZ = (int)floorf(playerPos.z / (CHUNK_SIZE * VOXEL_SIZE));
    
    for (auto& [key, chunk] : chunks) {
        // Distance-based culling - only render chunks within reasonable range
        int chunkDistX = abs(chunk.cx - playerChunkX);
        int chunkDistZ = abs(chunk.cz - playerChunkZ);
        int chunkDist = (int)sqrtf(chunkDistX * chunkDistX + chunkDistZ * chunkDistZ);
        
        // Skip chunks beyond 8 chunks for performance (increased from 4)
        if (chunkDist > 8) continue;
        
        // Frustum culling - only render visible chunks (DISABLED FOR TESTING)
        // if (!IsChunkVisible(camera, chunk.cx, chunk.cy, chunk.cz)) {
        //     continue;
        // }
        
        // ONLY render chunks with generated meshes
        if (chunk.meshGenerated && chunk.mesh.vertexCount > 0) {
            // TEMP: Render terrain as individual cubes to bypass mesh rendering issues
            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        BlockType block = chunk.getBlock(x, y, z);
                        if (block != BlockType::AIR) {
                            Vector3 blockPos = {
                                (float)(chunk.cx * CHUNK_SIZE + x),
                                (float)(chunk.cy * CHUNK_HEIGHT + y),
                                (float)(chunk.cz * CHUNK_SIZE + z)
                            };
                            
                            // Only render visible faces (simple check)
                            bool renderTop = (y == CHUNK_HEIGHT - 1) || (chunk.getBlock(x, y + 1, z) == BlockType::AIR);
                            bool renderBottom = (y == 0) || (chunk.getBlock(x, y - 1, z) == BlockType::AIR);
                            bool renderFront = (z == CHUNK_SIZE - 1) || (chunk.getBlock(x, y, z + 1) == BlockType::AIR);
                            bool renderBack = (z == 0) || (chunk.getBlock(x, y, z - 1) == BlockType::AIR);
                            bool renderRight = (x == CHUNK_SIZE - 1) || (chunk.getBlock(x + 1, y, z) == BlockType::AIR);
                            bool renderLeft = (x == 0) || (chunk.getBlock(x - 1, y, z) == BlockType::AIR);
                            
                            // Draw cube if any face is visible
                            if (renderTop || renderBottom || renderFront || renderBack || renderRight || renderLeft) {
                                DrawCube(blockPos, 0.9f, 0.9f, 0.9f, WHITE);
                            }
                        }
                    }
                }
            }
            
            chunksRendered++;
        }
    }
    
    if (chunksRendered > 0) {
        fprintf(stderr,"Drew %d chunks, total chunks: %d\n", chunksRendered, (int)chunks.size());
    }
    
    // Debug: Print player position and nearby chunks
    fprintf(stderr,"Player pos: (%.1f, %.1f, %.1f), Player chunk: (%d, %d)\n", 
           playerPos.x, playerPos.y, playerPos.z, playerChunkX, playerChunkZ);
    
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
    if (chunk.meshGenerated && chunk.mesh.vertexCount > 0) {
        UnloadMesh(chunk.mesh);
    }
    
    chunk.mesh = VoxelMesher::GenerateChunkMesh(chunk, &chunks);
    chunk.meshGenerated = true;
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
            result.first->second.meshGenerated = true;
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
