//
// Created by Danny on 2023-02-07.
//

#ifndef DENDER_SCENEACCELERATIONSTRUCTURE_HPP
#define DENDER_SCENEACCELERATIONSTRUCTURE_HPP

#include <vuk/Context.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/CommandBuffer.hpp>
#include <atomic>
#include "Assets/Primitive.hpp"
#include "Assets/Scene.hpp"

class SceneAccelerationStructure {
public:
    SceneAccelerationStructure(vuk::Context& context, vuk::Allocator& allocator);
    ~SceneAccelerationStructure();

    struct AllocatedAS {
        vuk::Unique<VkAccelerationStructureKHR> handle{};
        vuk::BufferCreateInfo memory{};
    };

    struct TLAS {
        AllocatedAS as{};
        vuk::Buffer buffer;
        std::span<VkSemaphore> build_completed{};


        vuk::CommandBuffer transfer_cmd;
        vuk::CommandBuffer compute_cmd;
    };

    struct BLAS {
        std::vector<AllocatedAS> entries{};

        vuk::Buffer buffer{};

        std::unordered_map<Mesh, uint32_t> mesh_indices;
    };

private:
    vuk::Allocator& allocator;
    vuk::Context& context;

    // These buffers may be reused even if the GPU is busy using the previous acceleration structure

    // Upload to instance_buffer
    vuk::Buffer* instance_scratch_buffer{};

    // Instance buffer for TLAS creation
    vuk::Buffer* instance_buffer{};

    // Signals that the upload to the instance buffer is done
    std::span<VkSemaphore> instance_upload_sempahore{};

    // Each in-flight frame needs one TLAS, since we're continiously updating these
    std::array<TLAS, 2> top_level;
    // Index in the array of TLAS's. Indicates the TLAS used for this frame.
    uint32_t tlas_index = 0;

    // Scratch memory for TLAS build
    vuk::Buffer tlas_scratch;

    // We can share a single BLAS across frames because updates will be done asynchronously
    BLAS bottom_level();

    // When a BLAS update is done, the result is stored here
    BLAS update_blas();

    // Signals that the BLAS update is completed
    std::atomic<bool> blas_update_done = false;

    // These must be deleted before checking blas_update_done and reassigning the new BLAS
    std::vector<BLAS> blas_deletion_queue{};

    // Protects access to the BLAS deletion queue
    std::mutex queue_mutex{};

    // Immediately destroy a TLAS
    void destroy(TLAS& tlas);

    // Immediately destroy a BLAS
    void destroy(BLAS& blas);

    // Queue deletion of a BLAS. Always use this instead of calling destroy() directly.
    void queue_delete(BLAS const& blas);

    // Process the BLAS deletion queue and destroys each element.
    void process_deletion_queue();

    // Returns true if a BLAS update is needed for the current BLAS
    bool must_update_blas(const Scene& scene);

    // Do a BLAS rebuild on a specific thread. This function is synchronous.
    void do_blas_rebuild(const std::vector<Mesh>& meshes, uint32_t const thread);

    void do_tlas_build(const Scene& scene);
};


#endif //DENDER_SCENEACCELERATIONSTRUCTURE_HPP
