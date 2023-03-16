//
// Created by Danny on 2023-02-06.
//

#include "RaytracingBuilder.hpp"
#include <vuk/Context.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/AllocatorHelpers.hpp>
#include <vuk/CommandBuffer.hpp>
#include <vuk/RenderGraph.hpp>
#include <vuk/Future.hpp>
#include <vulkan/vulkan.h>

void RaytracingBuilder::buildBlas(std::vector <RaytracingBuilder::BlasInput> &blases,
                                  VkBuildAccelerationStructureFlagsKHR flags) {
    // TODO: Seperate this into it's own function
    auto& context = allocator.get_context();
    uint32_t nbBlas = static_cast<uint32_t>(blases.size());
    VkDeviceSize asTotalSize{0}; // Memory size of all allocated BLAS
    uint32_t nbCompactions{0}; // Nb of BLAS requesting compaction
    VkDeviceSize maxScratchSize{0}; // Largest scratch size

    std::vector<BuildAccelerationStructure> buildAs(nbBlas);
    for (uint32_t i = 0; i < nbBlas; i++)
    {
        auto& inputBlas = blases[i];
        vuk::Unique<VkAccelerationStructureKHR> blas;

        //blasBuildInfo.dstAccelerationStructure = *blas;
        buildAs[i].BuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildAs[i].BuildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildAs[i].BuildInfo.flags = inputBlas.Flags | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildAs[i].BuildInfo.geometryCount = (uint32_t)inputBlas.AsGeometry.size();
        buildAs[i].BuildInfo.pGeometries = inputBlas.AsGeometry.data();

        // Build range information
        buildAs[i].RangeInfo = inputBlas.AsBuildRangeInfo.data();

        // Finding sizes of create acceleration structures and scratch
        std::vector<uint32_t> maxPrimCount(inputBlas.AsBuildRangeInfo.size());
        for (auto j = 0; j < blases[i].AsBuildRangeInfo.size(); j++)
        {
            maxPrimCount[j] = blases[i].AsBuildRangeInfo[j].primitiveCount;
        }

        context.vkGetAccelerationStructureBuildSizesKHR(
                context.device,
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buildAs[i].BuildInfo,
                maxPrimCount.data(),
                &buildAs[i].SizeInfo);

        // Extra info
        asTotalSize += buildAs[i].SizeInfo.accelerationStructureSize;
        maxScratchSize = std::max(maxScratchSize, buildAs[i].SizeInfo.buildScratchSize);
        nbCompactions += (buildAs[i].BuildInfo.flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR) ==
                         VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
        buildAs[i].Mesh = blases[i].Mesh;
    }

    // allocate the scratch buffers holding the temporary data of the acceleration structure
    // builder
    auto scratchBuffer = *vuk::allocate_buffer(
            allocator, vuk::BufferCreateInfo {
                    .mem_usage = vuk::MemoryUsage::eGPUonly,
                    .size = maxScratchSize,
            });

    // allocate a query pool for storing the needed size for every BLAS compaction.
    VkQueryPool queryPool{VK_NULL_HANDLE};
    if (nbCompactions > 0)
    {
        assert(nbCompactions == nbBlas); // Don't allow mix of on/off compaction
        VkQueryPoolCreateInfo qpci{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        qpci.queryCount = nbBlas;
        qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
        vkCreateQueryPool(context.device, &qpci, nullptr, &queryPool);
    }

    // TODO: Vuk specification says to push this as a
    // pass in the rendergraph
    // Batching creation / compaction of BLAS to allow staying in restricted amount of memory
    std::vector<uint32_t> indices;
    VkDeviceSize batchSize{0};
    VkDeviceSize batchLimit{256'000'000}; // 256MB
    /**
    for (uint32_t i = 0; i < nbBlas; i++)
    {
        indices.push_back(i);
        batchSize += buildAs[i].SizeInfo.accelerationStructureSize;
        // Over the limit or last BLAS element
        if (batchSize >= batchLimit || i == nbBlas - 1) {
            //vuk::CommandBuffer cmdBuf();
            cmdCreateBlas(
                    indices,
                    buildAs,
                    scratchBuffer->device_address,
                    allocator,
                    queryPool
            );

            if (queryPool)
            {
            }

            // Reset
            batchSize = 0;
            indices.clear();
        }
    }
    **/
    for (uint32_t i = 0; i < nbBlas; i++) {
        cmdCreateBlas(
                indices,
                buildAs,
                scratchBuffer->device_address,
                allocator,
                queryPool
                );
    }

    for (auto& b: buildAs)
    {
        m_blas.emplace_back(b.As);
    }

    vkDestroyQueryPool(device, queryPool, nullptr);
    allocator.deallocate(
            { &*scratchBuffer, 1 }
            );
}

vuk::Future RaytracingBuilder::cmdCreateBlas(std::vector<uint32_t>& indices,
                                      std::vector<BuildAccelerationStructure>& buildAS,
                                      uint64_t &scratchBufferAddress,
                                      vuk::Allocator allocator,
                                      VkQueryPool queryPool) {

    vuk::RenderGraph as_build("as_build");

    if (queryPool) // For querying compact size
    {
        vkResetQueryPool(device,
                         queryPool,
                         0,
                         static_cast<uint32_t>(indices.size()));
    }

    // We attach the vertex and index futures, which will be used for the BLAS build
    as_build.add_pass({
            .resources = {
                    "blas_buf"_buffer >> vuk::eAccelerationStructureBuildWrite,
                    "verts"_buffer >> vuk::eAccelerationStructureBuildRead,
                    "inds"_buffer >> vuk::eAccelerationStructureBuildRead
                    },
            .execute = [indices, &buildAS, &allocator, scratchBufferAddress, queryPool](vuk::CommandBuffer& commandBuffer)
        {
            uint32_t queryCnt{0};
            for (const auto &i: indices) {
                // Actual allocation of buffer and acceleration structure
                VkAccelerationStructureCreateInfoKHR createInfo{
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR
                };
                createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                createInfo.size = buildAS[i].SizeInfo.accelerationStructureSize; // Will be used to allocate memory
                allocator.allocate_acceleration_structures(
                        {&buildAS[i].As.As, 1},
                        {&createInfo, 1}
                        );

                // BuildInfo #2 part
                buildAS[i].BuildInfo.dstAccelerationStructure = buildAS[i].As.As;
                buildAS[i].BuildInfo.scratchData.deviceAddress = scratchBufferAddress;

                // Building the BLAS
                commandBuffer.build_acceleration_structures(
                        1,
                        &buildAS[i].BuildInfo,
                        &buildAS[i].RangeInfo
                        );

                // Make pipeline barrier
                commandBuffer.memory_barrier(
                        vuk::eAccelerationStructureBuildWrite,
                        vuk::eAccelerationStructureBuildRead
                        );
            }
    }
    });

    return vuk::Future(
            std::make_shared<vuk::RenderGraph>(std::move(as_build)),
                    "tlas_buf+"
                    );
}

RaytracingBuilder::RaytracingBuilder(vuk::Allocator& allocator, VkDevice& vkDevice) : allocator(allocator), device(vkDevice) {
    //this->context = context;
}


