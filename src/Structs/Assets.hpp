//
// Created by Danny on 2023-03-13.
//

#ifndef DENDER_ASSETS_HPP
#define DENDER_ASSETS_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <vuk/Buffer.hpp>
#include <vuk/Future.hpp>
namespace Asset {
	struct Buffer {
		std::vector<uint8_t>                    data;
		std::optional<vuk::Unique<vuk::Buffer>> vk_buffer;
		vuk::Future				                future;
	};

	struct BufferView {
		uint64_t count;
		uint64_t offset;
		uint64_t stride;
		vuk::Format format;
		Buffer* buffer;
	};
	struct IndexView : BufferView {
		vuk::IndexType format;
		uint64_t first_index;
	};
	struct Image {
		uint64_t id;
	};

	// Abstractions to raw "assets"
	struct Material {
		uint64_t albedo_texture;
		uint64_t normal_texture;
	};
	struct Mesh {
		BufferView positions;
		IndexView indices;
		BufferView normals;
		BufferView tangents;
		std::string name;
		uint64_t first_index;
	};
	struct Node {
		std::vector<Mesh*> meshes;
		Node* parent;
	};
	struct Scene {
		std::vector<Node>     nodes;
		std::vector<Mesh>     meshes;
		std::vector<Buffer*>  buffers;
		std::vector<Material> materials;
	};
}

#endif//DENDER_ASSETS_HPP
