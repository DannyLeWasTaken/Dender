//
// Created by Danny on 2023-03-02.
//
// https://github.com/spnda/fastgltf/blob/main/examples/gl_viewer/gl_viewer.cpp

#include "GltfScene.hpp"

#include <iostream>
#include <vuk/Partials.hpp>
#include <vuk/Buffer.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include "../stb_image.h"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

glm::mat4 get_transform_matrix(const fastgltf::Node& node, glm::mat4x4& base) {
    if (const auto* pMatrix = std::get_if<fastgltf::Node::TransformMatrix>(&node.transform)) {
        return base * glm::mat4x4(glm::make_mat4x4(pMatrix->data()));
    } else if (const auto* pTransform = std::get_if<fastgltf::Node::TRS>(&node.transform)) {
        return base
               * glm::translate(glm::mat4(1.0f), glm::make_vec3(pTransform->translation.data()))
               * glm::toMat4(glm::make_quat(pTransform->rotation.data()))
               * glm::scale(glm::mat4(1.0f), glm::make_vec3(pTransform->scale.data()));
    } else {
        return base;
    }
}

GltfScene::GltfScene(std::filesystem::path& path, vuk::Allocator& allocator) {
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
        gltfAsset = gltf->getParsedAsset();
    }
    //std::cout << "Size: " << gltfAsset->buffers.size() << std::endl;

    this->allocate(allocator);

    for (auto& node: this->gltfAsset->scenes[0].nodeIndices) {
        this->load_node(node, glm::mat4(1.0f));
    }
}

std::vector<vuk::Future> GltfScene::allocate(vuk::Allocator& allocator) {
    auto& buffers = gltfAsset->buffers;
    //std::vector<vuk::Future> futures;
    gltfBuffers.reserve(buffers.size());

    for (auto& buffer: buffers) {
        std::visit(overloaded {
            [](auto& arg) {},
            [&](fastgltf::sources::Vector& vector) {
                auto [vuk_buffer, future] = vuk::create_buffer(allocator, vuk::MemoryUsage::eGPUonly, vuk::DomainFlagBits::eTransferOnGraphics, std::span(vector.bytes)  );
                futures.emplace_back(future);
                this->m_bda.emplace_back(vuk_buffer->device_address);
                this->m_buffers.emplace_back(std::move(vuk_buffer));
                this->m_gltf_buffers.emplace_back(vector.bytes.data());
            },
            [&](fastgltf::sources::CustomBuffer& customBuffer) {
                //this->m_bda.emplace_back(customBuffer.id);
            }
        }, buffer.data);
    }
    return futures;
}

template<typename T>
std::vector<T> bytes_to_numbers(const std::vector<unsigned char>& bytes) {
    static_assert(std::is_same<T, float>::value || std::is_same<T, double>::value, "Only float or double is allowed");
    static_assert(bytes.size() % sizeof(T) == 0, "The size of the input bytes is incompatible with the requested type");

    std::vector<T> result;
    result.reserve(bytes.size() / sizeof(T));
    for (size_t i = 0; i < bytes.size(); i += sizeof(T)) {
        T value;
        std::memcpy(&value, bytes.data() + i, sizeof(T));
        result.push_back(value);
    }

    return result;
}

bool GltfScene::load_mesh(fastgltf::Mesh& mesh, glm::mat4& matrix) {
    this->m_transformations.emplace_back(matrix);
    for (size_t i = 0; i < mesh.primitives.size(); i++) {
        auto& prim = mesh.primitives[i];
        auto& material = this->gltfAsset->materials[prim.materialIndex.value()];
    }

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
            case fastgltf::ComponentType::UnsignedInt: return vuk::IndexType::eUint32;
            case fastgltf::ComponentType::UnsignedShort : return vuk::IndexType::eUint16;
            default: return vuk::IndexType::eNoneKHR;
        }
    };

    for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it) {
        GltfMesh outMesh = {};
        if (it->attributes.find("POSITION") == it->attributes.end())
            continue;


        // Only support indexed geometry
        if (!it->indicesAccessor.has_value()) {
            return false;
        }

        auto index = std::distance(mesh.primitives.begin(), it);
        {
            // Position
            auto &position_accessor = gltfAsset->accessors[it->attributes["POSITION"]];
            if (!position_accessor.bufferViewIndex.has_value())
                continue;

            auto &position_view = gltfAsset->bufferViews[position_accessor.bufferViewIndex.value()];

            vuk::VertexInputAttributeDescription vert_attrib;
            vert_attrib.offset = position_accessor.byteOffset;
            vert_attrib.format = get_internal_format(position_accessor.componentType,
													   position_accessor.type);
            vert_attrib.location = 0; // layout (location = 0)
            vert_attrib.binding = position_view.bufferIndex;
			if (!position_view.byteStride.has_value()) {
				outMesh.stride = fastgltf::getElementByteSize(position_accessor.type, position_accessor.componentType);
			} else {
				outMesh.stride = position_view.byteStride.value();
			}
            outMesh.vertex_attributes.emplace_back(vert_attrib);
            outMesh.vertex_buffer = &this->m_buffers[position_view.bufferIndex];
            std::cout << "Offset accessor is: " << position_accessor.byteOffset << std::endl;
            std::cout << "Offset view is: " << position_view.byteOffset << std::endl;
            std::cout << "Stride is: " << outMesh.stride << std::endl;


            if (vert_attrib.format == vuk::Format::eUndefined) {
                std::cout << mesh.name << " is not it!" << std::endl;
                //continue;
            }
        }
        {
            // Index
            auto& indices_accessor = this->gltfAsset->accessors[it->indicesAccessor.value()];
            if (!indices_accessor.bufferViewIndex.has_value())
                return false;

            auto& indices_view = this->gltfAsset->bufferViews[indices_accessor.bufferViewIndex.value()];
			outMesh.m_indices_count = indices_accessor.count;
            outMesh.index_buffer_type = get_index_format(indices_accessor.componentType);
            outMesh.m_indices_offset = indices_accessor.byteOffset;
			outMesh.m_first_index = (indices_accessor.byteOffset + indices_view.byteOffset) /
									fastgltf::getElementByteSize(indices_accessor.type, indices_accessor.componentType);
			std::cout << "Index buffer offset: " << outMesh.m_indices_offset << std::endl;
            if (outMesh.index_buffer_type == vuk::IndexType::eNoneKHR) {
                std::cout << "Buffer type is none!" << std::endl;
                continue;
            }
            outMesh.index_buffer = &this->m_buffers[indices_view.bufferIndex];
        }
        this->m_mesh.emplace_back(outMesh);
    }
    return true;
}

void GltfScene::load_node(size_t node_index, glm::mat4 matrix) {
    auto& node = this->gltfAsset->nodes[node_index];
    matrix = get_transform_matrix(node, matrix);

    if (node.meshIndex.has_value()) {
        this->load_mesh(this->gltfAsset->meshes[node.meshIndex.value()], matrix);
    }

    for (auto& child : node.children) {
        load_node(child, matrix);
    }
}

bool GltfScene::load_texture(fastgltf::Image &image) {
    auto getInternalFormat = [](int channels) {
        switch (channels) {
            case 2: return vuk::Format::eR8G8Srgb;
            case 3: return vuk::Format::eR8G8B8Srgb;
            case 4: return vuk::Format::eR8G8B8A8Srgb;
            default: return vuk::Format::eR8G8B8Srgb;
        }
    };

    Texture texture;
    vuk::Texture vuk_texture;
    std::visit(overloaded {
        [](auto& arg) {},
        [&](fastgltf::sources::FilePath& filePath) {
            assert(filePath.fileByteOffset == 0);
            int width, height, nrChannels;
            auto path = filePath.path.string();
            unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

            stbi_image_free(data);
        }
    }, image.data);

    this->m_textures.emplace_back(texture);

    return true;
}