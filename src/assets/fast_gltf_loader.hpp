//
// Created by Danny on 2023-03-08.
//

#ifndef DENDER_FAST_GLTF_LOADER_HPP
#define DENDER_FAST_GLTF_LOADER_HPP

#include <filesystem>
#include <string>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <fastgltf_parser.hpp>
#include <fastgltf_types.hpp>

#include <vuk/Buffer.hpp>

#include "../Structs/Assets.hpp"

class fast_gltf_loader {
public:
    Asset::Scene load_file(const std::filesystem::path& path, vuk::Allocator& allocator);
private:
    void load_node(std::unique_ptr<fastgltf::Asset> gltf_asset, fastgltf::Node& node);
    void load_mesh(std::unique_ptr<fastgltf::Asset> gltf_asset, fastgltf::Mesh& mesh);
	Result<Asset::Mesh> load_primitive(std::unique_ptr<fastgltf::Asset>& gltf_asset,
												Asset::Scene* scene,
												fastgltf::Primitive& primitive);
};


#endif //DENDER_FAST_GLTF_LOADER_HPP
