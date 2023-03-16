//
// Created by Danny on 2023-03-08.
//

#include "fast_gltf_loader.hpp"
#include "vuk/Partials.hpp"
#include <vuk/Future.hpp>

#include <iostream>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

Asset::Scene fast_gltf_loader::load_file(const std::filesystem::path& path, vuk::Allocator& allocator) {
	Asset::Scene scene;
	std::unique_ptr<fastgltf::Asset> gltf_asset;

	{
		fastgltf::Parser parser =
				fastgltf::Parser(fastgltf::Extensions::KHR_mesh_quantization);
		fastgltf::GltfDataBuffer dataBuffer;
		std::unique_ptr<fastgltf::glTF> gltf;

		if (!dataBuffer.loadFromFile(path)) {
			std::cerr << "Failed to load: " << path.string() << std::endl;
		}

		constexpr auto gltfOptions =
				fastgltf::Options::LoadGLBBuffers |
				fastgltf::Options::LoadExternalBuffers;
		auto type = fastgltf::determineGltfFileType(&dataBuffer);

		if (type == fastgltf::GltfType::glTF) {
			gltf = parser.loadGLTF(&dataBuffer, path.parent_path(), gltfOptions);
		} else if (type == fastgltf::GltfType::GLB) {
			gltf = parser.loadBinaryGLTF(&dataBuffer, path.parent_path(), gltfOptions);
		} else {
			std::cerr << "Failed to determine glTF container" << std::endl;
		}

		if (parser.getError() != fastgltf::Error::None) {
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(parser.getError()) << std::endl;
		}

		auto error = gltf->parse(fastgltf::Category::Scenes);
		if (error != fastgltf::Error::None) {
			std::cerr << "Failed to parse glTF: " << fastgltf::to_underlying(error) << std::endl;
		}
		gltf_asset = gltf->getParsedAsset();
	}

	for (auto& buffer: gltf_asset->buffers) {
		// TODO: RECODE THIS!!!!
		std::visit(overloaded {
						   [](auto& arg) {},
						   [&](fastgltf::sources::Vector& vector) {
							   auto [vk_buffer, future] = vuk::create_buffer(allocator,
																			  vuk::MemoryUsage::eGPUonly,
																			  vuk::DomainFlagBits::eTransferOnGraphics,
																			  std::span(vector.bytes));

							   scene.buffers.emplace_back(Asset::Buffer{
									   .data = vector.bytes,
									   .vk_buffer = std::move(vk_buffer),
									   .future = future
							   });
						   }
				   }, buffer.data);
	}

	// Load materials
	// TODO: Load materials

	// Load meshes
	for (auto& mesh: gltf_asset->meshes) {
		for (auto& primitive: mesh.primitives) {
			auto asset_mesh = load_primitive(gltf_asset, scene, primitive);
			scene.meshes.emplace_back(asset_mesh);
		}
	}

	return scene;
}
void fast_gltf_loader::load_node(std::unique_ptr<fastgltf::Asset> gltf_asset,
								 fastgltf::Node& node) {
}
void fast_gltf_loader::load_mesh(std::unique_ptr<fastgltf::Asset> gltf_asset, fastgltf::Mesh& mesh) {
	Asset::Mesh out_mesh{};

}
Asset::Mesh fast_gltf_loader::load_primitive(std::unique_ptr<fastgltf::Asset>& gltf_asset,
									  Asset::Scene& scene,
									  fastgltf::Primitive& primitive) {
	Asset::Mesh out_mesh{};

	auto get_internal_format = [](
									   fastgltf::ComponentType componentType,
									   fastgltf::AccessorType  accessorType) {
		switch (accessorType) {
			case fastgltf::AccessorType::Scalar:
				switch (componentType) {
					case fastgltf::ComponentType::Byte:   return vuk::Format::eR8Snorm;
					case fastgltf::ComponentType::Short:  return vuk::Format::eR16Sfloat;
					case fastgltf::ComponentType::Float:  return vuk::Format::eR32Sfloat;
					case fastgltf::ComponentType::Double: return vuk::Format::eR64Sfloat;

					case fastgltf::ComponentType::UnsignedByte:  return vuk::Format::eR8Unorm;
					case fastgltf::ComponentType::UnsignedShort: return vuk::Format::eR16Uint;
					case fastgltf::ComponentType::UnsignedInt:   return vuk::Format::eR32Uint;
					case fastgltf::ComponentType::Int:           return vuk::Format::eR32Sint;
					default:								     return vuk::Format::eUndefined;
				}
			case fastgltf::AccessorType::Vec2:
				switch(componentType) {
					case fastgltf::ComponentType::Byte:   return vuk::Format::eR8G8Snorm;
					case fastgltf::ComponentType::Short:  return vuk::Format::eR16G16Sfloat;
					case fastgltf::ComponentType::Float:  return vuk::Format::eR32G32Sfloat;
					case fastgltf::ComponentType::Double: return vuk::Format::eR64G64Sfloat;

					case fastgltf::ComponentType::UnsignedByte:  return vuk::Format::eR8G8Unorm;
					case fastgltf::ComponentType::UnsignedShort: return vuk::Format::eR16G16Uint;
					case fastgltf::ComponentType::UnsignedInt:   return vuk::Format::eR32G32Uint;
					case fastgltf::ComponentType::Int:           return vuk::Format::eR32G32Sint;
					default:									 return vuk::Format::eUndefined;
				}
			case fastgltf::AccessorType::Vec3:
				switch (componentType) {
					case fastgltf::ComponentType::Byte:   return vuk::Format::eR8G8B8Snorm;
					case fastgltf::ComponentType::Short:  return vuk::Format::eR16G16B16Sfloat;
					case fastgltf::ComponentType::Float:  return vuk::Format::eR32G32B32Sfloat;
					case fastgltf::ComponentType::Double: return vuk::Format::eR64G64B64Sfloat;

					case fastgltf::ComponentType::UnsignedByte:  return vuk::Format::eR8G8B8Unorm;
					case fastgltf::ComponentType::UnsignedShort: return vuk::Format::eR16G16B16Uint;
					case fastgltf::ComponentType::UnsignedInt:   return vuk::Format::eR32G32B32Uint;
					case fastgltf::ComponentType::Int:           return vuk::Format::eR32G32B32Sint;
					default:								     return vuk::Format::eUndefined;
				}
			case fastgltf::AccessorType::Vec4:
				switch (componentType) {
					case fastgltf::ComponentType::Byte:   return vuk::Format::eR8G8B8A8Snorm;
					case fastgltf::ComponentType::Short:  return vuk::Format::eR16G16B16A16Sfloat;
					case fastgltf::ComponentType::Float:  return vuk::Format::eR32G32B32A32Sfloat;
					case fastgltf::ComponentType::Double: return vuk::Format::eR64G64B64A64Sfloat;

					case fastgltf::ComponentType::UnsignedByte:  return vuk::Format::eR8G8B8A8Unorm;
					case fastgltf::ComponentType::UnsignedShort: return vuk::Format::eR16G16B16A16Uint;
					case fastgltf::ComponentType::UnsignedInt:   return vuk::Format::eR32G32B32A32Uint;
					case fastgltf::ComponentType::Int:           return vuk::Format::eR32G32B32A32Sint;
					default:								     return vuk::Format::eUndefined;
				}
			default: return vuk::Format::eUndefined;
		}
	};
	auto get_index_format = [](fastgltf::ComponentType format) {
		switch (format) {
			case fastgltf::ComponentType::UnsignedInt:   return vuk::IndexType::eUint32;
			case fastgltf::ComponentType::UnsignedShort: return vuk::IndexType::eUint16;
			case fastgltf::ComponentType::UnsignedByte:  return vuk::IndexType::eUint8EXT;
			default: return vuk::IndexType::eNoneKHR;
		}
	};

	{
		// Position
		Asset::BufferView& position_buffer = out_mesh.positions;
		auto& position_accessor = gltf_asset->accessors[primitive.attributes["POSITION"]];
		if (position_accessor.bufferViewIndex.has_value())
			return out_mesh;

		auto &position_view = gltf_asset->bufferViews[position_accessor.bufferViewIndex.value()];
		auto offset = position_accessor.byteOffset;

		position_buffer.count  = position_accessor.count;
		position_buffer.offset = offset;
		position_buffer.format = get_internal_format(position_accessor.componentType,
													 position_accessor.type);
		position_buffer.buffer = &scene.buffers[position_accessor.bufferViewIndex.value()];

		if (position_view.byteStride.has_value()) {
			position_buffer.stride = position_view.byteStride.value();
		} else {
			// .type defines vec3, vec2, scalar, and etc...
			// .componentType defines that type said vec is i.e. double (dvec), float (vec), and etc...
			position_buffer.stride =
					fastgltf::getElementByteSize(position_accessor.type, position_accessor.componentType);
		}
	}

	{
		// Index
		auto& indices_accessor = gltf_asset->accessors[primitive.indicesAccessor.value()];
		Asset::IndexView& index_buffer = out_mesh.indices;
		if (!indices_accessor.bufferViewIndex.has_value())
			return out_mesh;
		auto& indices_view = gltf_asset->bufferViews[indices_accessor.bufferViewIndex.value()];

		index_buffer.count       = indices_accessor.count;
		index_buffer.first_index = (indices_accessor.byteOffset + indices_view.byteOffset)
								   / fastgltf::getElementByteSize(
											 indices_accessor.type,
											 indices_accessor.componentType
											 );
		index_buffer.format      = get_index_format(indices_accessor.componentType);
		index_buffer.buffer      = &scene.buffers[indices_accessor.bufferViewIndex.value()];
	}

	{
		// Normals
		// TODO: Add normals :)
	}

	{
		// Tangents
	}

	{
		// Materials
	}
	return out_mesh;
}
