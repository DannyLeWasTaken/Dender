//
// Created by Danny on 2023-06-24.
//

#ifndef DENDER_SCENE_ACCELERATION_STRUCTURE_HPP
#define DENDER_SCENE_ACCELERATION_STRUCTURE_HPP

#include <vuk/Context.hpp>
#include <unordered_map>
#include "Structs/Assets.hpp"
#include "util/handle.hpp"

class scene_acceleration_structure {
public:
	/**
	 * @brief Create the acceleration structure manager
	 * @param allocator Reference to the memory allocator
	 */
	scene_acceleration_structure(vuk::Allocator& allocator);

	~scene_acceleration_structure();

	/**
	 * @brief Updates the acceleration structure for this frame. TLAS updates
	 * are instant, while a BLAS update may have a couple frames delay
	 * @param scene Reference to the scene
	 */
	void update();

	/**
	 * @brief Get the acceleration structure to use for rendering. For correct
	 * results, update() must be called first. Note that the acceleration structure
	 * build still be in progress.
	 */
	 VkAccelerationStructureKHR get_acceleration_structure() const;

	 struct AllocatedAS {
		 VkAccelerationStructureKHR handle{};
		 vuk::Buffer memory{};
	 };

	 struct TLAS {
		 // Memory slice of the AS
		 AllocatedAS as{};
		 // We will use one buffer for each TLAS
		 vuk::Buffer buffer{};
	 };

	 struct BLAS {
		 std::vector<AllocatedAS> entries{};
		 vuk::Buffer buffer{};
		 std::unordered_map<Handle<Asset::Mesh>, uint32_t> mesh_indices;
	 };

private:
	vuk::Allocator& allocator;

	vuk::Buffer instance_scratch_buffer{};
	vuk::Buffer instance_buffer{};
};


#endif//DENDER_SCENE_ACCELERATION_STRUCTURE_HPP
