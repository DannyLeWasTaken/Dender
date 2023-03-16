//
// Created by Danny on 2023-02-07.
//

#include "SceneAccelerationStructure.hpp"

SceneAccelerationStructure::SceneAccelerationStructure(vuk::Context& context,
                                                       vuk::Allocator &allocator) {
    this->allocator = allocator;
    this->context   = std::move(context);

    for (auto& tlas: top_level)
    {
        allocator.allocate_semaphores(tlas.build_completed);
    }

    allocator.allocate_semaphores(instance_upload_sempahore);
}

SceneAccelerationStructure::~SceneAccelerationStructure() {

}

namespace impl {
    struct BLASEntry {
        VkAccelerationStructureGeometryKHR geometry{};
        VkAccelerationStructureBuildRangeInfoKHR range{};
    };

    struct BLASBuildInfo {
        VkAccelerationStructureBuildGeometryInfoKHR geometry{};
        VkAccelerationStructureBuildSizesInfoKHR sizes{};

        // Offset of this BLAS entry in the main BLAS buffer.
        VkDeviceSize buffer_offset = 0;
        // Offset of this BLAS entry in the scratch buffer.
        VkDeviceSize scratch_offset = 0;
    };

    struct BLASBuildResources {
        vuk::Buffer scratch_buffer{};
        VkDeviceAddress scratch_address{};

        VkQueryPool compacted_size_qp{};
    };

    // Build a vector of BLAS entries. Assumes each mesh handle in the given meshes array is unique.
    // also updates the index map.
    std::vector<BLASEntry> get_blas_entries(vuk::Context& ctx, std::vector<Mesh>& meshes, std::unordered_map<Mesh, uint32_t>& index_map) {
        std::vector<BLASEntry> entries;
        entries.reserve(meshes.size());

        for (Mesh& mesh: meshes) {
            BLASEntry entry{};
            //vk_structure_type_acceleration_structure_geometry_khr
            entry.geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            // todo: mark meshes as opaque/non-opaque. (optimal for tracing is opaque).
            // vk_geometry_no_duplicate_any_hit_invocation_bit_khr
            entry.geometry.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR | VK_GEOMETRY_OPAQUE_BIT_KHR;
            //vkaccelerationstructuregeometrytrianglesdatakhr
            entry.geometry.geometry.triangles = VkAccelerationStructureGeometryTrianglesDataKHR{
                // vk_structure_type_acceleration_structure_geometry_triangles_data_khr
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .pNext = nullptr,
                // this format only describes position data
                .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
                .vertexData = {mesh.VertexBuffer.device_address},
                // todo: more flexible vertex stride/formats in engine?
                .vertexStride = (3 + 3 + 3 + 2) * sizeof(float),
                .maxVertex = (uint32_t)mesh.Vertices.size(),
                .indexType = VK_INDEX_TYPE_UINT32,
                .indexData = {mesh.IndexBuffer.device_address},
                // leave transform empty, as transforms are per-instance.
                .transformData = {}
            };
            entry.geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            entry.range = VkAccelerationStructureBuildRangeInfoKHR{
                .primitiveCount = (uint32_t)mesh.Indices.size() / 3,
                .primitiveOffset = 0,
                .firstVertex = 0,
                .transformOffset = 0
            };
            // store mesh->index mapping
            uint32_t const index = entries.size();
            index_map[mesh] = index;
            // store blas entry
            entries.push_back(entry);
        }
        return entries;
    }
}
