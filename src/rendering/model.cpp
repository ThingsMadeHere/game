#include "model.h"
#include <raymath.h>
#include <cstring>

bool ModelInstance::Load(const std::string& objPath, const std::string& texPath) {
    // Load the model from .obj file
    model = LoadModel(objPath.c_str());
    
    if (model.meshCount == 0) {
        loaded = false;
        return false;
    }
    
    // Load texture if provided
    if (!texPath.empty()) {
        texture = LoadTexture(texPath.c_str());
        // Apply texture to all materials
        for (int i = 0; i < model.materialCount; i++) {
            model.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
        }
    }
    
    loaded = true;
    return true;
}

void ModelInstance::Unload() {
    if (loaded) {
        UnloadModel(model);
        if (texture.id != 0) {
            UnloadTexture(texture);
        }
        loaded = false;
    }
}

void ModelInstance::Draw() {
    if (!loaded) return;
    
    // Build transform matrix
    Matrix translation = MatrixTranslate(position.x, position.y, position.z);
    Matrix rotX = MatrixRotateX(rotation.x * DEG2RAD);
    Matrix rotY = MatrixRotateY(rotation.y * DEG2RAD);
    Matrix rotZ = MatrixRotateZ(rotation.z * DEG2RAD);
    Matrix scaling = MatrixScale(scale, scale, scale);
    
    // Combine: T * R * S
    Matrix transform = MatrixMultiply(scaling, MatrixMultiply(rotZ, MatrixMultiply(rotY, MatrixMultiply(rotX, translation))));
    
    DrawMesh(model.meshes[0], model.materials[0], transform);
}

void ModelManager::AddModel(const std::string& name, const std::string& objPath,
                            const std::string& texPath, Vector3 position,
                            Vector3 rotation, float scale) {
    ModelInstance instance;
    instance.name = name;
    instance.position = position;
    instance.rotation = rotation;
    instance.scale = scale;
    
    if (instance.Load(objPath, texPath)) {
        models.push_back(instance);
    }
}

void ModelManager::RemoveModel(size_t index) {
    if (index < models.size()) {
        models[index].Unload();
        models.erase(models.begin() + index);
    }
}

void ModelManager::DrawAll() {
    for (auto& model : models) {
        model.Draw();
    }
}

void ModelManager::Clear() {
    for (auto& model : models) {
        model.Unload();
    }
    models.clear();
}

ModelInstance* ModelManager::GetModel(size_t index) {
    if (index < models.size()) {
        return &models[index];
    }
    return nullptr;
}
