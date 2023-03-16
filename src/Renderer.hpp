//
// Created by Danny on 2023-02-08.
//

#ifndef DENDER_RENDERER_HPP
#define DENDER_RENDERER_HPP

#include <string>
#include <memory>

namespace vuk {
    class RenderGraph;
    class Name;
    class Allocator;
}

class Renderer {
public:
    vuk::Name& render_name;

    Renderer(vuk::Allocator& inAllocator,
             vuk::Name& name);
    ~Renderer();

    auto Setup();
    auto Render();
    auto Cleanup();

protected:
    std::shared_ptr<vuk::RenderGraph> render_graph;
    vuk::Allocator&  allocator;
};


#endif //DENDER_RENDERER_HPP
