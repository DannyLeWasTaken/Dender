//
// Created by Danny on 2023-03-07.
//

#ifndef DENDER_ASSETS_H
#define DENDER_ASSETS_H

#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec3 TexCoord;
    int Id;
};

struct Texture {
    std::string Type;
    std::string Path;
    int Id;
};

struct Material {
    Texture albedoTexture;
    Texture emissiveTexture;
};

struct Primitive {
    std::vector<Vertex> Vertices;
    std::vector<unsigned int> Indices;
    std::vector<Texture> Textures;
    Material material;

    glm::mat4 WorldMatrix;

    std::string Name;
    uint64_t VertexOffset;
    uint64_t FirstIndex;
    uint64_t MaterialIndex;
};

struct Mesh {
    std::vector<Primitive> Primitives;
};

struct Scene {
    std::vector<Mesh> Meshes;

    std::vector<glm::vec3> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec2> m_texcoords;
    std::vector<Material> m_materials;
};

#endif //DENDER_ASSETS_H
