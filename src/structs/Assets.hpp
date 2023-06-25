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
#include "../util/handle_manager.hpp"

namespace Asset {
	struct Buffer {
		std::vector<uint8_t>                    data;
		vuk::Unique<vuk::Buffer> 				vk_buffer;
		vuk::Future				                future;
	};

	struct BufferView {
		uint64_t count;
		uint64_t offset_buffer_view;
        uint64_t offset_accessor;
		uint64_t stride;
		vuk::Format format;
		std::shared_ptr<Buffer> buffer;
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
            //this->positions.buffer->vk_buffer.reset();
            //this->indices.buffer->vk_buffer.reset();
            //this->normals.buffer->vk_buffer.reset();
            //this->tangents.buffer->vk_buffer.reset();
        }
	};
	struct Node {
		std::vector<Mesh*> meshes;
		Node* parent;
	};
	struct Scene {
	public:
		HandleManager<Mesh> mesh_handler;
		HandleManager<Node> node_handler;
		HandleManager<Buffer> buffer_handler;
		HandleManager<Material> material_handler;

		/**
		 * @brief Simply removes a mesh from the scene however,
		 * DOES NOT DESTROY the mesh itself
		 * @param handle& Handle of the mesh to remove
		 */
        std::shared_ptr<Mesh> remove_mesh(Handle<Mesh>& handle) {
			return this->mesh_handler.remove(handle).unwrap();
        }

		/**
		 * @brief Same functionality as remove_mesh, but destroys the mesh
		 * @param handle& Mesh to be destroyed
		 * @return void
		 */
		void destroy_mesh(Handle<Mesh>& handle) {
			this->mesh_handler.destroy(handle);
		}

        Handle<Mesh> add_mesh(const Mesh& mesh) {
			Handle<Mesh> handle = this->mesh_handler.add(mesh);
			return handle;
        }

        ~Scene() {
			this->mesh_handler.destroy_all(); // Destroy all meshes in scene

        }

	private:
	};
}

#endif//DENDER_ASSETS_HPP
