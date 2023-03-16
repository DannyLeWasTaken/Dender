//
// Created by Danny on 2023-02-08.
//

#include "Renderer.hpp"

#include <vuk/RenderGraph.hpp>
#include <vuk/Allocator.hpp>
#include <vuk/AllocatorHelpers.hpp>
#include <string>

Renderer::Renderer(vuk::Allocator &inAllocator, vuk::Name& inName): allocator(inAllocator), render_name(inName) {
    render_graph = std::make_shared<vuk::RenderGraph>(vuk::RenderGraph(inName.c_str()));
}
