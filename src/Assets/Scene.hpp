//
// Created by Danny on 2023-01-15.
//

#ifndef DENDER_SCENE_HPP
#define DENDER_SCENE_HPP

#include <vuk/Allocator.hpp>
#include <vuk/Partials.hpp>

#include "Mesh.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <vector>

class Scene {
public:
    Scene(std::string& path);
    void AllocateMeshes(vuk::Allocator allocator, std::shared_ptr<std::vector<vuk::Future>> futures);
    std::vector<Mesh*> Meshes;

    ~Scene();
private:
    const aiScene* assimpScene;
    std::vector<aiMesh> assimpMeshes;
    std::vector<std::unique_ptr<Texture>> loadedTextures;
    std::vector<std::unique_ptr<Texture>> loadMaterialTextures(vuk::Allocator allocator,
                                              aiMaterial* material,
                                              aiTextureType type);
    void processNode(aiNode* node, const aiScene* scene, vuk::Allocator allocator, std::shared_ptr<std::vector<vuk::Future>> futures);
    Mesh* processMesh(aiMesh* mesh, const aiScene* scene, vuk::Allocator allocator);
    std::string directory;
    std::string pathString;
    Assimp::Importer importer;
};


#endif //DENDER_SCENE_HPP
