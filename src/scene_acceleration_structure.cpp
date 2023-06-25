//
// Created by Danny on 2023-06-24.
//

#include "scene_acceleration_structure.hpp"

scene_acceleration_structure::scene_acceleration_structure(vuk::Allocator& allocator): allocator(allocator) {
}
scene_acceleration_structure::~scene_acceleration_structure() {

}

struct BLASEntry {
	VkAccelerationStructureGeometryKHR geometry{};
	VkAccelerationStructureBuildRangeInfoKHR range{};
};

struct BLASBuildInfo {
	VkAccelerationStructureBuildGeometryInfoKHR geometry{};
	VkAccelerationStructureBuildSizesInfoKHR sizes{};

	// Offset of this BLAS entry in the main BLAS buffer
	VkDeviceSize buffer_offset = 0;
	// Offset of this BLAS entry in the scratch buffer
	VkDeviceSize scratch_offset = 0;
};

struct BLASBuildResources {
	vuk::Buffer scratch_buffer{};
	VkDeviceAddress scratch_address{};

	VkQueryPool compacted_size_qp{};
};

std::vector<BLASEntry> get_blas_entires(vuk::Allocator& allocator,
										std::vector<Handle<Asset::Mesh>> const& meshes,
										std::unordered_map<Handle<Asset::Mesh>, uint32_t>& index_map ) {
	std::vector<BLASEntry> entries;
	entries.reserve(meshes.size());

	for (Handle<Asset::Mesh> handle: meshes) {
		Asset::Mesh const& mesh;
	}
}