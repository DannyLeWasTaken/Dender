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
		uint64_t offset_buffer_view;
        uint64_t offset_accessor;
		uint64_t stride;
		vuk::Format format;
		Buffer* buffer;
	};
	struct IndexView : BufferView {
		VkIndexType format;
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

        ~Mesh() {
            this->positions.buffer->vk_buffer.reset();
            this->indices.buffer->vk_buffer.reset();
            this->normals.buffer->vk_buffer.reset();
            this->tangents.buffer->vk_buffer.reset();
        }
	};
	struct Node {
		std::vector<Mesh*> meshes;
		Node* parent;
	};
	struct Scene {
		std::vector<Node>     nodes;
		std::vector<std::shared_ptr<Mesh>>     meshes;
		std::vector<Buffer*>  buffers;
		std::vector<Material> materials;

        std::unordered_map<std::string, std::shared_ptr<Mesh>> name_meshes;
        std::unordered_map<std::shared_ptr<Mesh>, std::string> mesh_names;

        void remove_mesh(std::shared_ptr<Mesh> mesh) {
            name_meshes.erase(mesh_names[mesh]);
            mesh_names.erase(mesh);
            mesh.reset();
        }

        void remove_mesh(const std::string& name) {
            auto mesh = name_meshes[name];
            mesh_names.erase(mesh);
            name_meshes.erase(name);
            mesh.reset();
        }

        std::shared_ptr<Mesh> add_mesh(Mesh mesh) {
            std::shared_ptr<Mesh> ptr_mesh = std::make_shared<Mesh>(mesh);
            name_meshes[ptr_mesh->name] = ptr_mesh;
            mesh_names[ptr_mesh] = ptr_mesh->name;
            meshes.push_back(ptr_mesh);
            return ptr_mesh;
        }

        ~Scene() {
            for (auto& mesh: this->mesh_names) {
                this->remove_mesh(mesh.first);
            }
        }
	};
}

#endif//DENDER_ASSETS_HPP
