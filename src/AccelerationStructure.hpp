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
	/// @brief An abstraction class for vulkan's accelerations structure
public:
    struct BlasInput
    {
		/// @brief Standard input expected to build a BLAS
        std::vector<VkAccelerationStructureGeometryKHR>       as_geometry;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> as_build_offset_info;
        VkBuildAccelerationStructureFlagsKHR                  flags{0};
    };
    struct BuildAccelerationStructure
    {
		/// @brief Class containing everything needed for the acceleration structure
        VkAccelerationStructureBuildGeometryInfoKHR     build_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        VkAccelerationStructureBuildSizesInfoKHR        size_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
		VkAccelerationStructureBuildRangeInfoKHR        range_info;
        VkAccelerationStructureKHR_T*         as; // result acceleration structure
    };

    explicit AccelerationStructure(std::optional<vuk::Allocator> allocator);
    ~AccelerationStructure();

    vuk::RenderGraph BuildBLAS(const std::vector<AccelerationStructure::BlasInput>& blas_input,
                   VkBuildAccelerationStructureFlagsKHR flags);
    void CreateBLAS(vuk::CommandBuffer& command_buffer,
                    std::vector<uint32_t> indices,
                    std::vector<BuildAccelerationStructure>& build_as,
                    VkDeviceAddress scratch_address);

    void AddTLAS();

    void InitalizeAS();

private:
    vuk::RenderGraph as_build_render_graph;
    vuk::Context&     context;
    std::optional<vuk::Allocator>   allocator;
};


#endif //DENDER_ACCELERATIONSTRUCTURE_HPP
