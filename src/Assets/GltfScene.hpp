//
// Created by Danny on 2023-03-02.
//

#ifndef DENDER_GLTFSCENE_HPP
#define DENDER_GLTFSCENE_HPP

#include <fastgltf_parser.hpp>
#include <fastgltf_types.hpp>
#include <fastgltf_util.hpp>
#include <vuk/Allocator.hpp>
#include <vuk/PipelineTypes.hpp>
#include <glm/mat4x4.hpp>
#include "../Structs/Assets.h"

struct GltfMesh {
    std::vector<vuk::VertexInputAttributeDescription> vertex_attributes;
    size_t stride;
    vuk::Unique<vuk::Buffer>* vertex_buffer;
    vuk::Unique<vuk::Buffer>* index_buffer;
    vuk::IndexType index_buffer_type;
    size_t m_indices_offset; // Offset in
	size_t m_indices_count;
	size_t m_first_index;
	size_t m_view_offset;
	size_t m_accessor_offset;
};

class GltfScene {
public:
    explicit GltfScene(std::filesystem::path& path, vuk::Allocator& allocator);

    void load_node(size_t node_index, glm::mat4 matrix);
    bool load_texture(fastgltf::Image& image);
    bool load_mesh(fastgltf::Mesh& mesh, glm::mat4& matrix);
    std::vector<vuk::Future> allocate(vuk::Allocator& allocator);

    std::vector<vuk::Unique<vuk::Buffer>> m_buffers;
    std::vector<uint64_t> m_bda; // Buffer device address
    std::vector<vuk::Future> futures;
    std::vector<GltfMesh> m_mesh;

private:
    std::unique_ptr<fastgltf::Asset> gltfAsset;
    std::vector<fastgltf::Buffer> gltfBuffers;
    std::vector<unsigned char*> m_gltf_buffers;
    std::vector<Texture> m_textures;
    std::vector<glm::mat4> m_transformations;
};

#endif //DENDER_GLTFSCENE_HPP
