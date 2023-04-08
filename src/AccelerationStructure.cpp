//
// Created by Danny on 2023-02-08.
//

#include "AccelerationStructure.hpp"
#include <vuk/Buffer.hpp>
#include <vuk/AllocatorHelpers.hpp>
#include <vuk/CommandBuffer.hpp>

AccelerationStructure::AccelerationStructure(vuk::Context inContext,
                                             vuk::Allocator inAllocator):
                                             context(std::move(inContext)),
                                             allocator(inAllocator) {

}


void AccelerationStructure::BuildBLAS(
        const std::vector<AccelerationStructure::BlasInput>& blas_input,
        VkBuildAccelerationStructureFlagsKHR flags) {

    std::vector<BuildAccelerationStructure> blases(blas_input.size());
    std::vector<vuk::Unique<vuk::Buffer>> blas_buffers;
    vuk::Unique<vuk::Buffer> blas_buffer;

    uint32_t max_scratch_size{0};
    uint32_t as_total_size{0};

    for (uint32_t i = 0; i < blas_input.size(); i++) {
        BuildAccelerationStructure& build_acceleration_structure = blases[i];
        const BlasInput& input = blas_input[i];

        VkAccelerationStructureBuildGeometryInfoKHR &build_info =
                build_acceleration_structure.build_info;
        build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build_info.flags = input.flags | flags;
        build_info.geometryCount = static_cast<uint32_t>(input.as_geometry.size());
        build_info.pGeometries = input.as_geometry.data();

        // Build range information
        build_acceleration_structure.range_info = *input.as_build_offset_info.data();

        // Finding sizes to create acceleration structures and scratch
        std::vector<uint32_t> max_primitive_count(input.as_build_offset_info.size());
        for (auto tt = 0; tt < input.as_build_offset_info.size(); tt++)
            max_primitive_count[tt] = input.as_build_offset_info[tt].primitiveCount;
        this->context.vkGetAccelerationStructureBuildSizesKHR(
                this->context.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &build_acceleration_structure.build_info,
                max_primitive_count.data(),
                &build_acceleration_structure.size_info
        );

        // Extra info
        max_scratch_size = (max_scratch_size > build_acceleration_structure.size_info.buildScratchSize) ? max_scratch_size :
                build_acceleration_structure.size_info.buildScratchSize;
        as_total_size += build_acceleration_structure.size_info.buildScratchSize;

        VkAccelerationStructureCreateInfoKHR blas_ci{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        blas_ci.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        blas_ci.size = build_acceleration_structure.size_info.accelerationStructureSize;
        blas_buffers[i] = *vuk::allocate_buffer(
                allocator,
                vuk::BufferCreateInfo{
                    .mem_usage = vuk::MemoryUsage::eGPUonly,
                    .size      = build_acceleration_structure.size_info.accelerationStructureSize
                }
                );
        blas_ci.buffer = blas_buffers[i]->buffer;
        blas_ci.offset = blas_buffers[i]->offset;

        build_acceleration_structure.as = vuk::Unique<VkAccelerationStructureKHR>(allocator);
        allocator.allocate_acceleration_structures({&*build_acceleration_structure.as, 1},
                                                   {&blas_ci, 1});
    }

    // Allocate the scratch buffers holding the temporary data of the acceleration structure builder
    auto scratch_buffer = *vuk::allocate_buffer(
            allocator,
            vuk::BufferCreateInfo{
                .mem_usage = vuk::MemoryUsage::eGPUonly,
                .size      = max_scratch_size
            }
            );
    blas_buffer = *vuk::allocate_buffer(
            allocator,
            vuk::BufferCreateInfo {
                .mem_usage = vuk::MemoryUsage::eGPUonly,
                .size = as_total_size
            }
            );
    blas_buffer;


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

    build_as.attach_buffer("blas_buffer", *blas_buffer);
    build_as.add_pass({
        .resources = {
                "blas_buffer"_buffer >> vuk::eAccelerationStructureBuildWrite
        },
        .execute = [blas_input, &blases](vuk::CommandBuffer& command_buffer) mutable {
            for (auto& blas: blases) {
                const auto* range_info = &blas.range_info;
                auto& build_info = blas.build_info;
                //command_buffer.build_acceleration_structure(1, range_info, build_info);
                command_buffer.build_acceleration_structures(1,
                                                             &build_info,
                                                             &range_info);
            }
        },
    });
}

void AccelerationStructure::CreateBLAS(vuk::CommandBuffer &command_buffer,
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
