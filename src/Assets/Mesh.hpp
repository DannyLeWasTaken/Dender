//
// Created by Danny on 2023-01-15.
//

#ifndef DENDER_MESH_HPP
#define DENDER_MESH_HPP

#include <vuk/Allocator.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/Image.hpp>
#include <vuk/Partials.hpp>

#include "../Structs/Texture.hpp"
#include "../Structs/Vertex.hpp"

#include <vector>

class Mesh {
public:
    Mesh(std::vector<Vertex> verts,
         std::vector<unsigned int> inds,
         std::vector<std::unique_ptr<Texture>> texs);

    std::vector<Vertex> Vertices;
    std::vector<unsigned int> Indices;
    std::vector<std::unique_ptr<Texture>> Textures;

    vuk::Buffer VertexBuffer;
    vuk::Buffer IndexBuffer;
    vuk::Buffer NormalBuffer;
    std::vector<vuk::Future> Futures;

    void Allocate(vuk::Allocator& allocator);
private:
};


#endif //DENDER_MESH_HPP
