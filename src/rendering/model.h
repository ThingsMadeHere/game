#pragma once
#include "raylib.h"
#include <vector>
#include <string>

// Model class for .obj files - NOT subject to marching cubes
// Models are standalone objects that use traditional .obj + texture rendering
class ModelInstance {
public:
    std::string name;
    Vector3 position;
    Vector3 rotation;
    float scale;
    
    // Loaded from file
    Model model;
    Texture2D texture;
    bool loaded;
    
    ModelInstance() : position({0,0,0}), rotation({0,0,0}), scale(1.0f), loaded(false) {}
    
    bool Load(const std::string& objPath, const std::string& texPath);
    void Unload();
    void Draw();
    bool IsLoaded() const { return loaded; }
};

// Manager for all model instances
class ModelManager {
private:
    std::vector<ModelInstance> models;
    
public:
    void AddModel(const std::string& name, const std::string& objPath, 
                  const std::string& texPath, Vector3 position, 
                  Vector3 rotation = {0,0,0}, float scale = 1.0f);
    void RemoveModel(size_t index);
    void DrawAll();
    void Clear();
    size_t Count() const { return models.size(); }
    ModelInstance* GetModel(size_t index);
};
