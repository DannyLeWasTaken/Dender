//
// Created by Danny on 2023-02-08.
//

#include "AccelerationStructure.hpp"
#include <vuk/Buffer.hpp>
#include <vuk/AllocatorHelpers.hpp>
#include <vuk/CommandBuffer.hpp>
#include <iostream>

AccelerationStructure::AccelerationStructure(
        std::optional<vuk::Allocator> inAllocator):
                                             context(inAllocator->get_context()),
                                             allocator(inAllocator) {

}


AccelerationStructure::SceneAccelerationStructure AccelerationStructure::build_blas(
        const std::vector<AccelerationStructure::BlasInput>& blas_input,
        VkBuildAccelerationStructureFlagsKHR flags) {

    std::vector<BuildAccelerationStructure> blases;
    std::vector<vuk::Unique<vuk::Buffer>> blas_buffers;
    vuk::Unique<vuk::Buffer> blas_buffer;

    uint32_t max_scratch_size{0};
    uint32_t as_total_size{0};

    for (uint32_t i = 0; i < blas_input.size(); i++) {
        const BlasInput& input = blas_input[i];

        VkAccelerationStructureBuildGeometryInfoKHR build_info = {
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR
        };
        build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build_info.flags = flags;
        build_info.geometryCount = static_cast<uint32_t>(input.as_geometry.size());
        build_info.pGeometries = input.as_geometry.data();

        // Build range information
        VkAccelerationStructureBuildRangeInfoKHR range_info =
                *input.as_build_offset_info.data();

        // Size information
        VkAccelerationStructureBuildSizesInfoKHR size_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};

        // Finding sizes to create acceleration structures and scratch
        std::vector<uint32_t> max_primitive_count(input.as_build_offset_info.size());
        for (auto tt = 0; tt < input.as_build_offset_info.size(); tt++) {
            max_primitive_count[tt] = input.as_build_offset_info[tt].primitiveCount;
            std::cout << max_primitive_count[tt] << std::endl;
        }

        VkDevice vk_device = this->context.device;

        this->context.vkGetAccelerationStructureBuildSizesKHR(
                vk_device,
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &build_info,
                max_primitive_count.data(),
                &size_info
        );

        // Extra info
        max_scratch_size = (max_scratch_size > size_info.buildScratchSize) ? max_scratch_size :
                size_info.buildScratchSize;
        as_total_size += size_info.buildScratchSize;

        VkAccelerationStructureCreateInfoKHR blas_ci{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        blas_ci.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        blas_ci.size = size_info.accelerationStructureSize;
        blas_buffers.emplace_back(*vuk::allocate_buffer(
                *allocator,
                vuk::BufferCreateInfo{
                    .mem_usage = vuk::MemoryUsage::eGPUonly,
                    .size      = blas_ci.size
                }
                ));
        blas_ci.buffer = blas_buffers[i]->buffer;
        blas_ci.offset = blas_buffers[i]->offset;

        BuildAccelerationStructure build_acceleration_structure {
                .build_info = build_info,
                .size_info =  size_info,
                .range_info = range_info,
                .as         = vuk::Unique<VkAccelerationStructureKHR>(*allocator)
        };

        allocator->allocate_acceleration_structures({&*build_acceleration_structure.as, 1},
                                                   {&blas_ci, 1});

        blases.emplace_back(std::move(build_acceleration_structure));
    }

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
    auto scratch_buffer = *vuk::allocate_buffer(
            *allocator,
            vuk::BufferCreateInfo{
                .mem_usage = vuk::MemoryUsage::eGPUonly,
                .size      = max_scratch_size
            }
            );
    blas_buffer = *vuk::allocate_buffer(
            *allocator,
            vuk::BufferCreateInfo {
                .mem_usage = vuk::MemoryUsage::eGPUonly,
                .size = as_total_size
            }
            );

    // Update build information
    for (uint32_t i = 0; i < blases.size(); i++) {
        BuildAccelerationStructure& build_acceleration_structure = blases[i];
        build_acceleration_structure.build_info.srcAccelerationStructure = VK_NULL_HANDLE;
        build_acceleration_structure.build_info.dstAccelerationStructure =
                *build_acceleration_structure.as;
        build_acceleration_structure.build_info.scratchData.deviceAddress =
                scratch_buffer->device_address;
    }


    // Batching the creation/compaction of BLAS to allow staying in restricted amount of memory
    vuk::RenderGraph build_as("blas_build");
    std::vector<uint32_t> indices;
    VkDeviceSize          batch_size{0};
    VkDeviceSize          batch_limit{256'000'000}; // 256 MB
    std::vector<VkAccelerationStructureGeometryKHR> as_geometry {blases.size()};

    build_as.attach_buffer("blas_buffer", *blas_buffer);
    for (auto& blas: blases) {
        as_geometry.push_back(*blas.build_info.pGeometries);
    }

    for (uint32_t i = 0; i < blases.size(); i++) {
        indices.push_back(i);
        batch_size += blases[i].size_info.accelerationStructureSize;

        if (batch_size >= batch_limit || i == blases.size() - 1) {
            struct render_as {
                VkAccelerationStructureBuildRangeInfoKHR        range_info;
                VkAccelerationStructureBuildGeometryInfoKHR     build_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
                VkAccelerationStructureGeometryKHR              as_geometry;
            };
            std::vector<render_as> index_blas;

            for (auto& index: indices) {
                render_as as{
                    .range_info =   blases[index].range_info,
                    .build_info =   blases[index].build_info,
                    .as_geometry = *blases[index].build_info.pGeometries
                };
                index_blas.emplace_back(as);
            }
            build_as.add_pass( {
                .resources { "blas_buffer"_buffer >> vuk::eAccelerationStructureBuildWrite, },
                .execute = [blases = index_blas](vuk::CommandBuffer& command_buffer) mutable {
                    for (uint32_t i = 0; i < blases.size(); i++) {
                        auto& blas = blases[i];
                        auto& range_info = blas.range_info;
                        auto& build_info = blas.build_info;
                        // Deal with dangling pointer here since ptr do not copy over their values
                        build_info.pGeometries = &blases[i].as_geometry;

                        const auto* pblas_offset = &range_info;
                        command_buffer.build_acceleration_structures(
                                1,
                                &build_info,
                                &pblas_offset);

                    }
                }
            } );

            indices.clear();
            batch_size = 0;
        }
    }

    return SceneAccelerationStructure {
        .graph = std::move(build_as),
        .acceleration_structures = std::move(blases)
    };
}

void AccelerationStructure::create_blas(vuk::CommandBuffer &command_buffer,
                                        std::vector<uint32_t> indices,
                                        std::vector<BuildAccelerationStructure> &build_as,
                                        VkDeviceAddress scratch_address) {
    for (const auto& index: indices) {
        VkAccelerationStructureCreateInfoKHR create_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        create_info.size = build_as[index].size_info.accelerationStructureSize;
        build_as[index].as;
    }
}

void AccelerationStructure::build_tlas() {

}
