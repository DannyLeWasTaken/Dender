//
// Created by Danny on 2023-01-15.
//
// Large portion of code ported from: https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/model.h
//

#include <stb_image.h>
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Scene.hpp"

Scene::Scene(std::string &path): pathString(path) {
    Assimp::Importer importer;

    // read file via assimp. Force triangles only and flipped uvs
    assimpScene = importer.ReadFile(path.c_str(),
                                    aiProcess_Triangulate |
                                    aiProcess_FlipUVs);
    if (!assimpScene || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
    || !assimpScene->mRootNode || !assimpScene->HasMaterials())
        std::cerr << "[ERROR::ASSIMP::" << path << "]: "
        << importer.GetErrorString() << std::endl;

    // Iterate over all meshes
    processNode(assimpScene->mRootNode, assimpScene);

    directory = path.substr(0, path.find_last_of('/'));
    std::cout << "LOADING SCENE: " << path.c_str() << std::endl;
}

void Scene::Allocate(vuk::Allocator allocator, std::vector<vuk::Future> *futures) {
    // Load all textures
    for (auto mesh: assimpMeshes)
    {
        Meshes.push_back(this->processMesh(mesh, assimpScene, allocator));
    }
}

void Scene::processNode(aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (mesh != nullptr)
            assimpMeshes.push_back(mesh);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh* Scene::processMesh(aiMesh *mesh, const aiScene *scene, vuk::Allocator allocator) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    std::cout << "Processing mesh" << std::endl;
    // Import all textures
    if (mesh->mTextureCoords != nullptr && mesh->mTextureCoords[0]) {
        std::cout << "Mesh-iter 0.9" << std::endl;
        aiTextureType textureTypes[] = {
                aiTextureType_BASE_COLOR,
                aiTextureType_EMISSIVE,
                aiTextureType_EMISSION_COLOR
        };
        std::cout << "Mesh-iter 1" << std::endl;
        for (auto &j: textureTypes) {
            std::cout << "Mesh-iter 1.1 | " << "Type: " << std::to_string(j) <<
                      " | Mesh Index: " << mesh->mMaterialIndex <<
                      " | numMaterial: " << assimpScene->mNumMaterials << std::endl;
            aiMaterial *material = assimpScene->mMaterials[mesh->mMaterialIndex];
            std::vector<Texture> texs = this->loadMaterialTextures(allocator, material, j);
            std::cout << "Mesh-iter 1.2" << std::endl;
        }
    }
    std::cout << "Mesh-iter 2" << std::endl;
    // Process all vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vert;
        vert.Position.x = mesh->mVertices[i].x;
        vert.Position.y = mesh->mVertices[i].y;
        vert.Position.z = mesh->mVertices[i].z;

        vert.Normal.x = mesh->mNormals[i].x;
        vert.Normal.y = mesh->mNormals[i].y;
        vert.Normal.z = mesh->mNormals[i].z;

        vertices.push_back(vert);
    }
    std::cout << "Mesh-iter 3" << std::endl;
    // Process all indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    std::cout << "Mesh-iter 3" << std::endl;
    Mesh* m = new Mesh(vertices, indices, std::move(textures));
    m->Allocate(allocator);
    return m;
}

std::vector<Texture> Scene::loadMaterialTextures(vuk::Allocator allocator, aiMaterial* material, aiTextureType type) {
    std::vector<Texture> textures;
    std::string fileName = pathString;
    fileName = directory + '/' + fileName;
    std::cout << "LMT 0.4 | " << std::to_string(type) << std::endl;
    auto a = aiGetMaterialTextureCount(material, type);
    auto maxAmount = material->GetTextureCount(type);
    std::cout << "LMT 0.5 | " << maxAmount << std::endl;
    for (unsigned int i = 0; i < maxAmount; i++)
    {
        aiString str;
        std::cout << "LMT 0.9 | " << std::to_string(type) << " | " << i << std::endl;

        material->GetTexture(type, i, &str);
        bool skip = false;
        std::cout << "LMT 1 | " << str.C_Str() << std::endl;
        std::cout << "LMT 1.1 | " << fileName << std::endl;
        for (auto & loadedTexture : loadedTextures)
        {
            if (std::strcmp(
                    loadedTexture.Path.data(),
                    str.C_Str()
                    ) == 0)
            {
                textures.push_back(std::move(loadedTexture));
                skip = true;
            }
        }
        if (skip)
            continue;
        std::cout << "LMT 2" << std::endl;
        Texture texture;
        texture.Id = (int)i;
        texture.Path = str.C_Str();
        texture.Type = std::to_string(type);
        std::cout << "LMT 3" << std::endl;

        // Load texture from path
        int width, height, nrComponents;

        unsigned char* data = stbi_load(fileName.c_str(),
                                        &width,
                                        &height,
                                        &nrComponents,
                                        0);
        std::cout << "LMT 4 | " << str.C_Str() << std::endl;

        if (!data)
            std::cerr << "Failed to load texture at path: " << str.C_Str() << std::endl;

        vuk::Format format;
        switch (nrComponents)
        {
            case 1:
                format = vuk::Format::eR8Srgb;
                break;
            case 2:
                format = vuk::Format::eR8G8Srgb;
                break;
            case 3:
                format = vuk::Format::eR8G8B8Srgb;
                break;
            case 4:
                format = vuk::Format::eR8G8B8Srgb;
                break;
        };
        std::cout << "LMT 5" << std::endl;

        auto [tex, texFuture] = vuk::create_texture(allocator,
                                                 format,
                                                 vuk::Extent3D{ (unsigned)width, (unsigned)height, 1u },
                                                 data,
                                                 true);
        std::cout << "LMT 6" << std::endl;
        texture.vukTexture = std::move(tex);
        textures.push_back(std::move(texture));
        std::cout << "LMT 7" << std::endl;
    }
    return textures;
}
