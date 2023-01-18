//
// Created by Danny on 2023-01-15.
//
// Large portion of code ported from: https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/model.h
//

#include <stb_image.h>
#include <iostream>
#include <assimp/Importer.hpp>
#include <utility>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Scene.hpp"

Scene::Scene(std::string &path): pathString(path) {
    // read file via assimp. Force triangles only and flipped uvs
    assimpScene = importer.ReadFile(path.c_str(),
                                    aiProcess_Triangulate |
                                    aiProcess_GenNormals |
                                    aiProcess_FlipUVs);
    if (!assimpScene || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
    || !assimpScene->mRootNode || !assimpScene->HasMaterials())
        std::cerr << "[ERROR::ASSIMP::" << path << "]: "
        << importer.GetErrorString() << std::endl;

    directory = path.substr(0, path.find_last_of('/'));
    std::cout << "LOADING SCENE: " << path.c_str() << std::endl;
}

void Scene::processNode(aiNode *node, const aiScene *scene, vuk::Allocator allocator, std::shared_ptr<std::vector<vuk::Future>> futures) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        std::cout << "Storing mesh " << i << std::endl;
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        Meshes.push_back(processMesh(mesh, scene, allocator));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, allocator, futures);
    }
}

void Scene::AllocateMeshes(vuk::Allocator allocator, std::shared_ptr<std::vector<vuk::Future>> futures) {
    processNode(assimpScene->mRootNode, assimpScene, allocator, std::move(futures));
    for (auto& mesh: Meshes)
    {
        mesh->Allocate(allocator);
    }
}

Mesh* Scene::processMesh(aiMesh* mesh, const aiScene *scene, vuk::Allocator allocator) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::unique_ptr<Texture>> textures;
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
    // Process all indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // Import all textures
    aiTextureType textureTypes[] = {
            aiTextureType_BASE_COLOR,
            aiTextureType_EMISSIVE,
            aiTextureType_EMISSION_COLOR
    };
    for (auto &j: textureTypes) {
        aiMaterial *material = assimpScene->mMaterials[mesh->mMaterialIndex];
        std::vector<std::unique_ptr<Texture>> texs = this->loadMaterialTextures(allocator, material, j);
        textures.insert(textures.end(),
                        std::make_move_iterator(texs.begin()),
                        std::make_move_iterator(texs.end()));
    }

    Mesh* m = new Mesh(vertices, indices, std::move(textures));
    m->Allocate(allocator);
    return m;
}

std::vector<std::unique_ptr<Texture>> Scene::loadMaterialTextures(vuk::Allocator allocator, aiMaterial* material, aiTextureType type) {
    std::vector<std::unique_ptr<Texture>> textures;
    for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
    {
        aiString str;
        std::cout << "Processing texture " << i << " | Type: " << std::to_string(type) << std::endl;
        material->GetTexture(type, i, &str);
        bool skip = false;
        for (auto & loadedTexture : loadedTextures)
        {
            if (std::strcmp(
                    loadedTexture->Path.data(),
                    str.C_Str()
                    ) == 0)
            {
                textures.push_back(std::move(loadedTexture));
                skip = true;
            }
        }
        if (skip)
            continue;
        std::unique_ptr<Texture> texture = std::make_unique<Texture>();
        texture->Id = (int)i;
        texture->Path = str.C_Str();
        texture->Type = std::to_string(type);

        // Generate path
        std::string fileName = std::string(str.C_Str());
        fileName = directory + "/" + fileName;

        std::cout << "Loading texture at path: " << fileName << std::endl;

        // Load texture from path
        int width, height, nrComponents;

        unsigned char* data = stbi_load(fileName.c_str(),
                                        &width,
                                        &height,
                                        &nrComponents,
                                        4);

        if (!data) {
            std::cerr << "Failed to load texture at path: " << fileName.c_str() << std::endl;
            continue;
        }

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
                format = vuk::Format::eR8G8B8A8Srgb;
                break;
            default:
                format = vuk::Format::eUndefined;
                break;
        };
        if (format == vuk::Format::eUndefined) {
            std::cerr << fileName.c_str() << " has an invalid texture type." << std::endl;
            continue;
        }

        auto [tex, texFuture] = vuk::create_texture(allocator,
                                                 vuk::Format::eR8G8B8A8Srgb,
                                                 vuk::Extent3D{ (unsigned)width, (unsigned)height,  1u},
                                                 data,
                                                 false);
        texture->vukTexture = std::move(tex);
        textures.push_back(std::move(texture));
        stbi_image_free(data);
    }
    return textures;
}

Scene::~Scene() {
    importer.FreeScene();
}
