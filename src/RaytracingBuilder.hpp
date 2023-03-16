//
// Created by Danny on 2023-02-06.
//

#ifndef DENDER_RAYTRACINGBUILDER_HPP
#define DENDER_RAYTRACINGBUILDER_HPP

#include <vulkan/vulkan.hpp>
#include <vuk/Allocator.hpp>
#include <vuk/Util.hpp>
#include <vuk/Context.hpp>
#include "./Structs/Assets.h"

class RaytracingBuilder {
public:
    // Inputs used to build Bottom-level acceleration structure.
    // You manage the lifetime of the buffer(s) referenced by the VkAccelerationStructureGeometryKHRs within.
    // In particular, you must make sure they are still valid and not being modified when the BLAS is built or updated.
    struct BlasInput
    {
        std::vector<VkAccelerationStructureGeometryKHR>       AsGeometry;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> AsBuildRangeInfo;
        VkBuildAccelerationStructureFlagsKHR                  Flags{0};
        const Primitive*                                           Mesh;
    };
    struct AccelerationKHR
    {
        VkAccelerationStructureKHR As;
        vuk::Buffer buffer;
        const Primitive* Mesh;
    };
    struct BuildAccelerationStructure
    {
        VkAccelerationStructureBuildGeometryInfoKHR BuildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        VkAccelerationStructureBuildSizesInfoKHR SizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        const VkAccelerationStructureBuildRangeInfoKHR* RangeInfo;
        AccelerationKHR As; // result acceleration structure
        AccelerationKHR CleanUpAS;
        const Primitive*     Mesh;
    };

    RaytracingBuilder(vuk::Allocator& allocator,
                      VkDevice&       device);

    void buildBlas(std::vector<RaytracingBuilder::BlasInput>& blases,
                   VkBuildAccelerationStructureFlagsKHR flags);

    vuk::Future cmdCreateBlas(
            std::vector<uint32_t>& indices,
            std::vector<BuildAccelerationStructure>& buildAS,
            uint64_t& scratchBufferAddress,
            vuk::Allocator allocator,
            VkQueryPool queryPool
            );

private:
    vuk::Allocator& allocator;
    VkDevice&       device;
    std::vector<AccelerationKHR> m_blas;
};


#endif //DENDER_RAYTRACINGBUILDER_HPP
