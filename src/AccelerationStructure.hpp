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
    struct BlasAccelerationStructure
    {
		/// @brief Class containing everything needed for the acceleration structure
        VkAccelerationStructureBuildGeometryInfoKHR     build_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        VkAccelerationStructureBuildSizesInfoKHR        size_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
		VkAccelerationStructureBuildRangeInfoKHR        range_info;
        vuk::Unique<VkAccelerationStructureKHR>         as; // result acceleration structure
        vuk::Unique<vuk::Buffer>                        buffer;
    };
    struct BLASSceneAccelerationStructure {
        vuk::RenderGraph graph;
        std::vector<BlasAccelerationStructure> acceleration_structures;
    };

    struct TlasAccelerationStructure {
        std::vector<VkAccelerationStructureInstanceKHR> instances;
        VkAccelerationStructureBuildGeometryInfoKHR     build_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        VkAccelerationStructureBuildSizesInfoKHR        size_info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
        vuk::Unique<VkAccelerationStructureKHR>         as;
        vuk::Unique<vuk::Buffer>                        instances_buffer;
        vuk::Unique<vuk::Buffer>                        buffer;
    };

    struct TlasSceneAccelerationStructure {
        vuk::RenderGraph graph;
        TlasAccelerationStructure acceleration_structure;
    };

    explicit AccelerationStructure(std::optional<vuk::Allocator> allocator);
    ~AccelerationStructure();

    BLASSceneAccelerationStructure build_blas(const std::vector<AccelerationStructure::BlasInput>& blas_input,
                                              VkBuildAccelerationStructureFlagsKHR flags);
    static void create_blas(vuk::CommandBuffer& command_buffer,
                     const std::vector<uint32_t>& indices,
                     std::vector<BlasAccelerationStructure>& build_as,
                     VkDeviceAddress scratch_address);

    TlasSceneAccelerationStructure build_tlas(
            std::vector<VkAccelerationStructureInstanceKHR>& build_acceleration_structure
            );

    void InitalizeAS();

private:
    vuk::RenderGraph as_build_render_graph;
    vuk::Context&     context;
    std::optional<vuk::Allocator>   allocator;
};


#endif //DENDER_ACCELERATIONSTRUCTURE_HPP
