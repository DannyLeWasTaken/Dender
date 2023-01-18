//
// Created by Danny on 2023-01-15.
//

#include "Mesh.hpp"

#include <utility>

Mesh::Mesh(std::vector<Vertex> verts,
           std::vector<unsigned int> inds,
           std::vector<std::unique_ptr<Texture>> texs):
    Vertices(std::move(verts)), Indices(std::move(inds)), Textures(std::move(texs)) {};

/*
 * @brief Allocates the mesh onto memory, but textures which is handled by the scene
 */
void Mesh::Allocate(vuk::Allocator &allocator) {
    auto [vertBuffer, vertFuture] = vuk::create_buffer(allocator,
                                                       vuk::MemoryUsage::eGPUonly,
                                                       vuk::DomainFlagBits::eTransferOnGraphics,
                                                       std::span(Vertices));
    VertexBuffer = *vertBuffer;

    auto [indBuffer, indFuture] = vuk::create_buffer(allocator,
                                                       vuk::MemoryUsage::eGPUonly,
                                                       vuk::DomainFlagBits::eTransferOnGraphics,
                                                       std::span(Indices));
    IndexBuffer = *indBuffer;

    auto [normBuffer, normFuture] = vuk::create_buffer(allocator,
                                                       vuk::MemoryUsage::eGPUonly,
                                                       vuk::DomainFlagBits::eTransferOnGraphics,
                                                       std::span(Vertices));
    NormalBuffer = *normBuffer;

    Futures.push_back(vertFuture);
    Futures.push_back(indFuture);
    Futures.push_back(normFuture);
};
