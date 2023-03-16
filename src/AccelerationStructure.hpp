//
// Created by Danny on 2023-02-08.
//

#ifndef DENDER_ACCELERATIONSTRUCTURE_HPP
#define DENDER_ACCELERATIONSTRUCTURE_HPP

#include <vuk/RenderGraph.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/Allocator.hpp>
#include <vector>

#include "./Structs/Assets.hpp"

class AccelerationStructure {
public:
    struct BlasInput
    {
        std::vector<VkAccelerationStructureGeometryKHR>       as_geometry;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> as_build_offset_info;
        VkBuildAccelerationStructureFlagsKHR                  flags{0};
    };
    struct BuildAccelerationStructure
    {
        VkAccelerationStructureBuildGeometryInfoKHR     build_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        VkAccelerationStructureBuildSizesInfoKHR        size_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
		VkAccelerationStructureBuildRangeInfoKHR        range_info;
        vuk::Unique<VkAccelerationStructureKHR>         as; // result acceleration structure
        vuk::Unique<VkAccelerationStructureKHR>         cleanup_as;
    };

    AccelerationStructure(vuk::Context context, vuk::Allocator allocator);
    ~AccelerationStructure();

    void AddBLAS(AccelerationStructure::BlasInput blas_input);

    void AddTLAS();

    void InitalizeAS();

private:
    vuk::RenderGraph as_build_render_graph;
    vuk::Context     context;
    vuk::Allocator   allocator;
};


#endif //DENDER_ACCELERATIONSTRUCTURE_HPP
