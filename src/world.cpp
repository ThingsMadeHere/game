#include "world.h"
#include "noise.h"
#include "voxel_renderer.h"
#include "gpu_renderer.h"
#include <cstdio>
#include <cmath>

// Simple frustum culling check
bool IsChunkVisible(const Camera3D& camera, int cx, int cy, int cz) {
    // Simple distance-based culling for now
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
    return distance < (RENDER_DISTANCE * CHUNK_SIZE * 2.0f); // Adjust visibility distance
}

// Global GPU renderer instance
GPUChunkRenderer gpuRenderer;

// Thread worker function
void ChunkWorker(World* world) {
    while (world->IsRunning()) {
        ChunkRequest req = world->PopChunkRequest();
        if (req.cx == -999) {
            break; // Shutdown signal
        }
        
        // Generate chunk
        ChunkKey key = {req.cx, req.cy, req.cz};
        Chunk chunk(req.cx, req.cy, req.cz);
        world->GenerateTerrain(chunk);
        
        // Add to world (terrain only, no mesh yet)
        std::lock_guard<std::mutex> lock(world->GetMutex());
        auto result = world->GetChunks().emplace(key, chunk);
        if (result.second) {
            // Mark chunk as having terrain but no mesh yet
            result.first->second.meshGenerated = false;
            result.first->second.needsUpdate = false;
            
            // Generate mesh after all chunks in the area are loaded
            // This ensures neighbors are available for proper boundary handling
            world->GenerateMesh(result.first->second);
        }
    }
}

// Helper function to get chunk without creating new ones
Chunk* World::GetChunkIfExists(int cx, int cy, int cz) {
    ChunkKey key = {cx, cy, cz};
    auto it = chunks.find(key);
    return (it != chunks.end()) ? &it->second : nullptr;
}

Chunk& World::GetChunk(int cx, int cy, int cz) {
    ChunkKey key = {cx, cy, cz};
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
    
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                float worldX = (float)(chunk.cx * CHUNK_SIZE + x) * VOXEL_SIZE;
                float worldY = (float)(chunk.cy * CHUNK_HEIGHT + y) * VOXEL_SIZE;
                float worldZ = (float)(chunk.cz * CHUNK_SIZE + z) * VOXEL_SIZE;
                
                // Generate proper continuous terrain height
                float groundHeight = 8.0f + SmoothNoise3D(worldX * 0.02f, 0, worldZ * 0.02f) * 6.0f;
                
                // Create solid ground below height, air above
                float density = 0.0f;
                unsigned char material = 0; // AIR by default
                
                if (worldY < groundHeight) {
                    density = 1.0f; // Solid ground
                    
                    // Generate random material based on noise
                    float materialNoise = SmoothNoise3D(worldX * 0.1f, worldY * 0.1f, worldZ * 0.1f);
                    
                    // Determine material based on depth and noise
                    if (worldY < groundHeight - 3.0f) {
                        // Underground materials
                        if (materialNoise > 0.7f) {
                            material = 8; // COAL (rare)
                        } else if (materialNoise > 0.6f) {
                            material = 9; // IRON (uncommon)
                        } else if (materialNoise > 0.55f && worldY < groundHeight - 6.0f) {
                            material = 10; // GOLD (rare, deep)
                        } else if (materialNoise > 0.5f && worldY < groundHeight - 8.0f) {
                            material = 11; // DIAMOND (very rare, very deep)
                        } else {
                            material = 1; // STONE (common)
                        }
                    } else if (worldY < groundHeight - 1.0f) {
                        // Sub-surface materials
                        if (materialNoise > 0.4f) {
                            material = 2; // DIRT
                        } else {
                            material = 1; // STONE
                        }
                    } else {
                        // Surface materials
                        if (materialNoise > 0.3f) {
                            material = 3; // GRASS
                        } else if (materialNoise > 0.2f) {
                            material = 4; // SAND
                        } else {
                            material = 3; // GRASS
                        }
                    }
                    
                    // Add some caves - hollow out areas below surface
                    if (worldY < groundHeight - 2.0f) {
                        float caveNoise = SmoothNoise3D(worldX * 0.05f, worldY * 0.05f, worldZ * 0.05f);
                        if (caveNoise > 0.6f) {
                            density = -1.0f; // Cave
                            material = 0; // AIR
                        }
                    }
                } else {
                    density = -1.0f; // Air
                    material = 0; // AIR
                }
                
                chunk.SetDensity(x, y, z, density);
                chunk.SetMaterial(x, y, z, material);
            }
        }
    }
    
    // Generate density texture for this chunk (black=air, white=block)
    chunk.GenerateDensityTexture();
    
    chunk.mesh = marchingCubes.GenerateMesh(chunk);
    chunk.meshGenerated = true; // Mark mesh as generated
    chunk.needsUpdate = false;  // Mark as clean
}

void World::UpdateChunk(int cx, int cy, int cz) {
    ChunkKey key = {cx, cy, cz};
    auto it = chunks.find(key);
    if (it != chunks.end()) {
        if (it->second.needsUpdate) {
            printf("Updating dirty chunk (%d,%d,%d)\n", cx, cy, cz);
            
            // Only regenerate mesh if chunk is dirty
            if (it->second.meshGenerated) {
                UnloadMesh(it->second.mesh);
                it->second.mesh = {0};
            }
            it->second.mesh = marchingCubes.GenerateMesh(it->second);
            it->second.meshGenerated = true;
            it->second.needsUpdate = false;
            
            // Also regenerate density texture if needed
            if (!it->second.densityTextureGenerated || it->second.needsUpdate) {
                it->second.GenerateDensityTexture();
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

void World::SetVoxel(int cx, int cy, int cz, int x, int y, int z, float density) {
    Chunk& chunk = GetChunk(cx, cy, cz);
    chunk.SetDensity(x, y, z, density);
    chunk.needsUpdate = true;
}

void World::Init() {
    running = true;
    
    // Start worker threads for chunk generation
    int numThreads = std::thread::hardware_concurrency();
    if (numThreads < 2) numThreads = 2;
    if (numThreads > 8) numThreads = 8; // Cap at 8 threads
    
    for (int i = 0; i < numThreads; i++) {
        workerThreads.emplace_back(ChunkWorker, this);
    }
    
    printf("Started %d worker threads for chunk generation\n", numThreads);
    
    gpuRenderer.Init();
}

void World::Cleanup() {
    // Stop worker threads
    running = false;
    
    // Send shutdown signals to all workers
    for (size_t i = 0; i < workerThreads.size(); i++) {
        QueueChunkGeneration(-999, -999, -999);
    }
    
    // Wait for all threads to finish
    for (auto& t : workerThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    gpuRenderer.Cleanup();
}

void World::Render(const Camera3D& camera) {
    int chunksRendered = 0;
    int voxelsRendered = 0;
    
    // Calculate player position for distance-based LOD
    Vector3 playerPos = camera.position;
    int playerChunkX = (int)floorf(playerPos.x / (CHUNK_SIZE * VOXEL_SIZE));
    int playerChunkZ = (int)floorf(playerPos.z / (CHUNK_SIZE * VOXEL_SIZE));
    
    for (auto& [key, chunk] : chunks) {
        // Distance-based culling - only render chunks within reasonable range
        int chunkDistX = abs(chunk.cx - playerChunkX);
        int chunkDistZ = abs(chunk.cz - playerChunkZ);
        int chunkDist = (int)sqrtf(chunkDistX * chunkDistX + chunkDistZ * chunkDistZ);
        
        // Skip chunks beyond 4 chunks for performance (REDUCED from 8)
        if (chunkDist > 4) continue;
        
        // Frustum culling - only render visible chunks
        if (!IsChunkVisible(camera, chunk.cx, chunk.cy, chunk.cz)) {
            continue;
        }
        
        chunksRendered++;
        
        // Use generated mesh if available (MUCH faster than individual voxels)
        if (chunk.meshGenerated && chunk.mesh.vertexCount > 0) {
            // Draw the entire chunk mesh in one call - this is the key optimization!
            gpuRenderer.RenderChunk(chunk, camera);
        } else {
            // Fallback to voxel rendering for chunks without meshes
            // Dynamic LOD based on distance from player
            int skipFactor = 1;
        if (chunkDist > 4) skipFactor = 2;  // Skip every other voxel
        if (chunkDist > 6) skipFactor = 4;  // Skip every 4th voxel
        
        // Render only visible voxels with lighting and shading
        for (int x = 0; x < CHUNK_SIZE; x += skipFactor) {
            for (int y = 0; y < CHUNK_HEIGHT; y += skipFactor) {
                for (int z = 0; z < CHUNK_SIZE; z += skipFactor) {
                    if (chunk.GetDensity(x, y, z) > 0.0f) {
                        // Quick visibility check - only check immediate neighbors
                        bool surrounded = true;
                        
                        // Simplified neighbor check (only check within current chunk)
                        if (x > 0 && chunk.GetDensity(x - 1, y, z) <= 0.0f) surrounded = false;
                        else if (x < CHUNK_SIZE - 1 && chunk.GetDensity(x + 1, y, z) <= 0.0f) surrounded = false;
                        else if (y > 0 && chunk.GetDensity(x, y - 1, z) <= 0.0f) surrounded = false;
                        else if (y < CHUNK_HEIGHT - 1 && chunk.GetDensity(x, y + 1, z) <= 0.0f) surrounded = false;
                        else if (z > 0 && chunk.GetDensity(x, y, z - 1) <= 0.0f) surrounded = false;
                        else if (z < CHUNK_SIZE - 1 && chunk.GetDensity(x, y, z + 1) <= 0.0f) surrounded = false;
                        else surrounded = false; // At chunk edge, assume visible
                        
                        // Only render if not completely surrounded
                        if (!surrounded) {
                            unsigned char material = chunk.GetMaterial(x, y, z);
                            Color baseColor = VoxelRenderer::GetBlockColor(material);
                            
                            // Calculate lighting based on face normals and sun position
                            Vector3 sunDirection = {0.7f, 0.7f, 0.2f}; // Sun from top-right
                            float brightness = 0.3f; // Ambient light
                            
                            // Check which faces are exposed and calculate lighting
                            float faceBrightness = 0.0f;
                            int exposedFaces = 0;
                            
                            // X- face (left)
                            if (x == 0 || chunk.GetDensity(x - 1, y, z) <= 0.0f) {
                                Vector3 normal = {-1.0f, 0.0f, 0.0f};
                                faceBrightness += fmaxf(0.0f, -sunDirection.x) * 0.8f;
                                exposedFaces++;
                            }
                            
                            // X+ face (right)
                            if (x == CHUNK_SIZE - 1 || chunk.GetDensity(x + 1, y, z) <= 0.0f) {
                                Vector3 normal = {1.0f, 0.0f, 0.0f};
                                faceBrightness += fmaxf(0.0f, sunDirection.x) * 0.8f;
                                exposedFaces++;
                            }
                            
                            // Y- face (bottom)
                            if (y == 0 || chunk.GetDensity(x, y - 1, z) <= 0.0f) {
                                Vector3 normal = {0.0f, -1.0f, 0.0f};
                                faceBrightness += fmaxf(0.0f, -sunDirection.y) * 0.6f; // Less bright on bottom
                                exposedFaces++;
                            }
                            
                            // Y+ face (top)
                            if (y == CHUNK_HEIGHT - 1 || chunk.GetDensity(x, y + 1, z) <= 0.0f) {
                                Vector3 normal = {0.0f, 1.0f, 0.0f};
                                faceBrightness += fmaxf(0.0f, sunDirection.y) * 1.0f; // Brightest on top
                                exposedFaces++;
                            }
                            
                            // Z- face (front)
                            if (z == 0 || chunk.GetDensity(x, y, z - 1) <= 0.0f) {
                                Vector3 normal = {0.0f, 0.0f, -1.0f};
                                faceBrightness += fmaxf(0.0f, -sunDirection.z) * 0.8f;
                                exposedFaces++;
                            }
                            
                            // Z+ face (back)
                            if (z == CHUNK_SIZE - 1 || chunk.GetDensity(x, y, z + 1) <= 0.0f) {
                                Vector3 normal = {0.0f, 0.0f, 1.0f};
                                faceBrightness += fmaxf(0.0f, sunDirection.z) * 0.8f;
                                exposedFaces++;
                            }
                            
                            // Average brightness for all exposed faces
                            if (exposedFaces > 0) {
                                brightness += faceBrightness / exposedFaces;
                            }
                            
                            // Apply lighting to color
                            Color litColor = {
                                (unsigned char)(baseColor.r * brightness),
                                (unsigned char)(baseColor.g * brightness),
                                (unsigned char)(baseColor.b * brightness),
                                baseColor.a
                            };
                            
                            Vector3 pos = {
                                (float)(chunk.cx * CHUNK_SIZE + x) * VOXEL_SIZE,
                                (float)(chunk.cy * CHUNK_HEIGHT + y) * VOXEL_SIZE,
                                (float)(chunk.cz * CHUNK_SIZE + z) * VOXEL_SIZE
                            };
                            
                            // Adjust voxel size based on LOD
                            float voxelSize = VOXEL_SIZE * skipFactor;
                            DrawCube(pos, voxelSize, voxelSize, voxelSize, litColor);
                            voxelsRendered++;
                        }
                    }
                }
            }
        }
    }
    
    // Draw chunk boundaries only for visible chunks
    for (int cx = -RENDER_DISTANCE; cx <= RENDER_DISTANCE; cx++) {
        for (int cz = -RENDER_DISTANCE; cz <= RENDER_DISTANCE; cz++) {
            if (IsChunkVisible(camera, cx, 0, cz)) {
                Vector3 chunkPos = {
                    (float)(cx * CHUNK_SIZE) * VOXEL_SIZE, 
                    0, 
                    (float)(cz * CHUNK_SIZE) * VOXEL_SIZE
                };
                Vector3 size = {
                    (float)CHUNK_SIZE * VOXEL_SIZE, 
                    1, 
                    (float)CHUNK_SIZE * VOXEL_SIZE
                };
                DrawCubeWires(chunkPos, size.x, size.y, size.z, RED);
            }
        }
    }
    
    printf("Detailed rendering: %d chunks, %d voxels\n", chunksRendered, voxelsRendered);
}
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
    chunk.mesh = marchingCubes.GenerateMesh(chunk, chunks);
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
        
        ChunkKey neighborKey = {nx, ny, nz};
        auto it = chunks.find(neighborKey);
        if (it != chunks.end() && it->second.meshGenerated) {
            // Regenerate this neighbor's mesh with updated neighbor information
            it->second.mesh = marchingCubes.GenerateMesh(it->second, chunks);
        }
    }
}

void World::ProcessChunkQueue() {
    // Process any pending chunks from the queue
    while (true) {
        ChunkRequest req = PopChunkRequest();
        if (req.cx == -999) break;
        
        // Check if chunk already exists
        ChunkKey key = {req.cx, req.cy, req.cz};
        if (chunks.find(key) != chunks.end()) continue;
        
        // Generate chunk
        Chunk chunk(req.cx, req.cy, req.cz);
        GenerateTerrain(chunk);
        
        // Add to world
        auto result = chunks.emplace(key, chunk);
        if (result.second) {
            result.first->second.mesh = marchingCubes.GenerateMesh(result.first->second);
            result.first->second.meshGenerated = true;
            result.first->second.needsUpdate = false;
        }
    }
}
