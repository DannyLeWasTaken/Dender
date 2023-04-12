//
// Created by Danny on 2023-02-08.
//

#include "AccelerationStructure.hpp"
#include <vuk/Buffer.hpp>
#include <vuk/AllocatorHelpers.hpp>
#include <vuk/CommandBuffer.hpp>
#include <vuk/Partials.hpp>
#include <iostream>
#include <VkBootstrap.h>

AccelerationStructure::AccelerationStructure(
        std::optional<vuk::Allocator> inAllocator):
                                             context(inAllocator->get_context()),
                                             allocator(inAllocator) {

}


AccelerationStructure::BLASSceneAccelerationStructure AccelerationStructure::build_blas(
        const std::vector<AccelerationStructure::BlasInput>& blas_input,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR) {

    std::vector<BlasAccelerationStructure> blases;

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
        auto blas_buffer = *vuk::allocate_buffer(
                *allocator,
                vuk::BufferCreateInfo{
                    .mem_usage = vuk::MemoryUsage::eGPUonly,
                    .size      = size_info.accelerationStructureSize
                }
                );
        blas_ci.buffer = blas_buffer->buffer;
        blas_ci.offset = blas_buffer->offset;

        BlasAccelerationStructure build_acceleration_structure {
                .build_info = build_info,
                .size_info  = size_info,
                .range_info = range_info,
                .as         = vuk::Unique<VkAccelerationStructureKHR>(*allocator),
                .buffer     = std::move(blas_buffer)
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

    // Update build information
    for (uint32_t i = 0; i < blases.size(); i++) {
        BlasAccelerationStructure& build_acceleration_structure = blases[i];
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

    //build_as.attach_buffer("blas_buffer", *blas_buffer);
    for (auto& blas: blases) {
        as_geometry.push_back(*blas.build_info.pGeometries);
    }

    for (uint32_t i = 0; i < blases.size(); i++) {
        indices.push_back(i);
        batch_size += blases[i].size_info.accelerationStructureSize;

        if (batch_size >= batch_limit || i == blases.size() - 1) {
            struct render_as {
                VkAccelerationStructureBuildRangeInfoKHR        range_info{};
                VkAccelerationStructureBuildGeometryInfoKHR     build_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
                VkAccelerationStructureGeometryKHR              as_geometry{};
            };
            //std::vector<render_as> index_blas;
            auto blas_buffer = vuk::allocate_buffer(
                    *allocator,
                    vuk::BufferCreateInfo {
                        .mem_usage = vuk::MemoryUsage::eGPUonly,
                        .size      = batch_size
                    });


            vuk::Query query;

            // TODO: THERE IS A 100% MORE INTELLIGENT WAY TO BUILD YOUR BLASES WITHOUT HAVING TO CREATE
            //  A FUCKING RENDERPASS FOR EACH ACCELERATION STRUCTURE.
            for (auto &index: indices) {
                render_as blas{
                    .range_info =   blases[index].range_info,
                    .build_info =   blases[index].build_info,
                    .as_geometry = *blases[index].build_info.pGeometries
                };

                build_as.attach_buffer("blas_buffer", *blases[index].buffer);
                build_as.add_pass({
                    .resources  = { "blas_buffer"_buffer >> vuk::eAccelerationStructureBuildWrite },
                    .execute    = [blas](vuk::CommandBuffer& command_buffer) mutable {
                        auto& range_info = blas.range_info;
                        auto& build_info = blas.build_info;
                        // Deal with dangling pointer here since ptr do not copy over their values
                        build_info.pGeometries = &blas.as_geometry;

                        const auto* pblas_offset = &range_info;
                        command_buffer.build_acceleration_structures(
                                1,
                                &build_info,
                                &pblas_offset);
                    }
                });
            }
            indices.clear();
            batch_size = 0;
        }
    }

    return BLASSceneAccelerationStructure {
        .graph = std::move(build_as),
        .acceleration_structures = std::move(blases)
    };
}

void AccelerationStructure::create_blas(vuk::CommandBuffer& command_buffer,
                                        const std::vector<uint32_t>& indices,
                                        std::vector<BlasAccelerationStructure> &build_as,
                                        VkDeviceAddress scratch_address) {

    for (const auto& index: indices) {
        VkAccelerationStructureCreateInfoKHR create_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        create_info.size = build_as[index].size_info.accelerationStructureSize;
        build_as[index].as;
    }
}

AccelerationStructure::TlasSceneAccelerationStructure AccelerationStructure::build_tlas(std::vector<VkAccelerationStructureInstanceKHR>& instances )
{
    std::vector<vuk::Unique<vuk::Buffer>*> blas_buffers;
    vuk::Unique<VkAccelerationStructureKHR> tlas;
    uint32_t instances_count = instances.size();

    std::cout << "Total instances: " << instances.size() << std::endl;

    // Create buffer that stores all instances
    auto [instances_buffer, instances_future] = vuk::create_buffer(
            *allocator,
            vuk::MemoryUsage::eCPUtoGPU,
            vuk::DomainFlagBits::eTransferOnGraphics,
            std::span(instances)
            );


    // TLAS creation

    // A wrapper that points to uploaded instances
    VkAccelerationStructureGeometryInstancesDataKHR instance_vk { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
    instance_vk.data.deviceAddress = instances_buffer->device_address;

    // Put created instances information into a VkAccelerationStructureGeometryKHR
    VkAccelerationStructureGeometryKHR as_geometry { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    as_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    as_geometry.geometry.instances = instance_vk;

    // Find sizes
    VkAccelerationStructureBuildGeometryInfoKHR tlas_build_info{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    tlas_build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    tlas_build_info.geometryCount = 1;
    tlas_build_info.pGeometries = &as_geometry;
    tlas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlas_build_info.srcAccelerationStructure = VK_NULL_HANDLE;


    VkAccelerationStructureBuildSizesInfoKHR tlas_size_info { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    context.vkGetAccelerationStructureBuildSizesKHR(
            allocator->get_context().device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &tlas_build_info, &instances_count, &tlas_size_info);

    // Allocate the TLAS object and a buffer that holds the data
    VkAccelerationStructureCreateInfoKHR tlas_ci{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    tlas_ci.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlas_ci.size = tlas_size_info.accelerationStructureSize;

    auto tlas_buffer = *vuk::allocate_buffer(*allocator, { .mem_usage = vuk::MemoryUsage::eGPUonly, .size = tlas_size_info.accelerationStructureSize });
    tlas_ci.buffer = tlas_buffer->buffer;
    tlas_ci.offset = tlas_buffer->offset;

    tlas = vuk::Unique<VkAccelerationStructureKHR>(*allocator);
    allocator->allocate_acceleration_structures({ &*tlas, 1 }, { &tlas_ci, 1 });

    // Allocate scratch memory & update build info
    auto tlas_scratch_buffer =
            *vuk::allocate_buffer(*allocator, vuk::BufferCreateInfo{ .mem_usage = vuk::MemoryUsage::eGPUonly, .size = tlas_size_info.buildScratchSize });
    tlas_build_info.scratchData.deviceAddress = tlas_scratch_buffer->device_address;
    tlas_build_info.dstAccelerationStructure  = *tlas;

    TlasAccelerationStructure tlases_acceleration_structure {
        .instances = instances,
        .build_info = tlas_build_info,
        .size_info = tlas_size_info,
        .as = std::move(tlas),
        .instances_buffer = std::move(instances_buffer),
        .buffer = std::move(tlas_buffer),
    };

    vuk::RenderGraph as_build("tlas_as_build");
    as_build.attach_buffer("tlas_buffer", *tlases_acceleration_structure.buffer );
    as_build.add_pass({
        .resources = {
                "tlas_buffer"_buffer >> vuk::eAccelerationStructureBuildWrite
        },
        .execute = [instances_count, as_geometry, tlas_build_info = tlases_acceleration_structure.build_info](vuk::CommandBuffer& command_buffer) mutable {
            // Make a copy of the top AS geometry to not dangle
            tlas_build_info.pGeometries = &as_geometry;

            // Describe what is being built
            VkAccelerationStructureBuildRangeInfoKHR tlas_offset{ instances_count , 0, 0, 0 };
            const VkAccelerationStructureBuildRangeInfoKHR* ptlas_offset = &tlas_offset;
            command_buffer.build_acceleration_structures(1, &tlas_build_info, &ptlas_offset);
        }
    });


    return AccelerationStructure::TlasSceneAccelerationStructure {
        .graph = std::move(as_build),
        .acceleration_structure = std::move(tlases_acceleration_structure)
    };
}
