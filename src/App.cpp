//
// Created by Danny on 2023-01-13.
//

#include "App.hpp"
#include "GlfwHelper.hpp"
#include "utils.hpp"

#include <vulkan/vulkan.h>
#include <iostream>
#include <vuk/Buffer.hpp>
#include <vuk/Future.hpp>
#include "Renderer.hpp"
//#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>

// Initialization of everything
App::App() {
    std::cout << "Start app!" << std::endl;

    vkb::InstanceBuilder builder;
    builder.request_validation_layers()
    .set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                           void* pUserData) -> VkBool32 {
        auto ms = vkb::to_string_message_severity(messageSeverity);
        auto mt = vkb::to_string_message_type(messageType);
        printf("[%s: %s](user defined)\n%s\n", ms, mt, pCallbackData->pMessage);
        return VK_FALSE;
    })
    .set_app_name(APP_TITLE.c_str())
    .set_engine_name("Danny's Renderer")
    .require_api_version(1, 2, 0)
    .set_app_version(0, 1, 0);

    auto inst_ret = builder.build();
    if (!inst_ret) {
        throw std::runtime_error("Couldn't initialize instance");
    }

    std::cout << "1" << std::endl;

    vkbInstance = inst_ret.value();
    vkb::PhysicalDeviceSelector selector { vkbInstance };
    window = GlfwHelper::create_window_glfw(APP_TITLE.c_str(), true);
    glfwSetWindowUserPointer(window, this);
    surface = GlfwHelper::create_surface_glfw(vkbInstance.instance, window);
    selector.set_surface(surface)
    .set_minimum_version(1, 0)
    .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
    .add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
    .add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
    .add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

    std::cout << "2" << std::endl;

    auto phys_ret = selector.select();
    vkb::PhysicalDevice vkbPhysicalDevice;
    if (!phys_ret) {
        std::cout << "Device does not support one or more of the following extensions:\n"
                                 " - VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME\n"
                                 " - VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME\n"
                                 " - VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME\n"
                                 " - VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME\n"
                                 "Common cause: Non-hardware accelerated ray tracing GPU" << std::endl;
    }
    vkbPhysicalDevice = phys_ret.value();
    physical_device = vkbPhysicalDevice.physical_device;

    std::cout << "2.5" << std::endl;

    vkb::DeviceBuilder deviceBuilder { vkbPhysicalDevice };
    VkPhysicalDeviceVulkan12Features vk12features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    vk12features.timelineSemaphore = true;
    vk12features.descriptorBindingPartiallyBound = true;
    vk12features.descriptorBindingUpdateUnusedWhilePending = true;
    vk12features.shaderSampledImageArrayNonUniformIndexing = true;
    vk12features.runtimeDescriptorArray = true;
    vk12features.descriptorBindingVariableDescriptorCount = true;
    vk12features.hostQueryReset = true;
    vk12features.bufferDeviceAddress = true;
    vk12features.shaderOutputLayer = true;
    vk12features.bufferDeviceAddress = true;
    vk12features.bufferDeviceAddressCaptureReplay = true;
    VkPhysicalDeviceVulkan11Features vk11features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
    vk11features.shaderDrawParameters = true;
    VkPhysicalDeviceFeatures2 vk10features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
    vk10features.features.shaderInt64 = true;
    VkPhysicalDeviceSynchronization2FeaturesKHR sync_feat{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
                                                           .synchronization2 = true };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
                                                                   .accelerationStructure = true };
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
                                                                     .rayTracingPipeline = true };
    deviceBuilder = deviceBuilder.
            add_pNext(&vk12features)
            .add_pNext(&vk11features)
            .add_pNext(&sync_feat)
            .add_pNext(&accelFeature)
            .add_pNext(&vk10features)
            .add_pNext(&rtPipelineFeature);

    auto dev_ret = deviceBuilder.build();
    if (!dev_ret) {
        std::cerr << "Couldn't create device" << std::endl;
        //throw std::runtime_error("Couldn't create device");
    }

    vkbDevice = dev_ret.value();
    graphics_queue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    auto graphics_queue_family_index = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    transfer_queue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
    auto transfer_queue_family_index = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();
    device = vkbDevice.device;

    std::cout << "3" << std::endl;

    vuk::ContextCreateParameters::FunctionPointers fps;
#define VUK_EX_LOAD_FP(name) fps.name = (PFN_##name)vkGetDeviceProcAddr(device, #name);
    VUK_EX_LOAD_FP(vkSetDebugUtilsObjectNameEXT);
    VUK_EX_LOAD_FP(vkCmdBeginDebugUtilsLabelEXT);
    VUK_EX_LOAD_FP(vkCmdEndDebugUtilsLabelEXT);
    VUK_EX_LOAD_FP(vkCmdBuildAccelerationStructuresKHR);
    VUK_EX_LOAD_FP(vkGetAccelerationStructureBuildSizesKHR);
    VUK_EX_LOAD_FP(vkCmdTraceRaysKHR);
    VUK_EX_LOAD_FP(vkCreateAccelerationStructureKHR);
    VUK_EX_LOAD_FP(vkDestroyAccelerationStructureKHR);
    VUK_EX_LOAD_FP(vkGetRayTracingShaderGroupHandlesKHR);
    VUK_EX_LOAD_FP(vkCreateRayTracingPipelinesKHR);

    context.emplace(vuk::ContextCreateParameters{
            vkbInstance,
            device,
            physical_device,
            graphics_queue,
            graphics_queue_family_index,
            VK_NULL_HANDLE,
            VK_QUEUE_FAMILY_IGNORED,
            transfer_queue,
            transfer_queue_family_index,
            fps
    });

    const unsigned num_inflight_frames = 3;
    vuk_device_sf_resource.emplace(*context, num_inflight_frames);
    vuk_allocator.emplace(*vuk_device_sf_resource);
    swapchain = context->add_swapchain(util::make_swapchain(vkbDevice, {}));
    present_ready = vuk::Unique<std::array<VkSemaphore, 3>>(*vuk_allocator);
    render_complete = vuk::Unique<std::array<VkSemaphore, 3>>(*vuk_allocator);

    vuk_allocator->allocate_semaphores(*present_ready);
    vuk_allocator->allocate_semaphores(*render_complete);
    vuk_futures = std::make_shared<std::vector<vuk::Future>>();

    acceleration_structure = new AccelerationStructure(
            vuk_allocator);

    std::cout << "Built!" << std::endl;
}

// Destruction of all
App::~App() {
    /**
    for (auto& scene: Scenes)
    {
        for (auto& mesh: scene->Meshes)
        {
            for (auto& texture: mesh->Textures)
            {
                std::cout << "Freed texture" << std::endl;
                //texture.reset();
            }
        }
    }
    **/
    std::cout << "Freed all textures" << std::endl;
    present_ready.reset();
    std::cout << "1" << std::endl;
    render_complete.reset();
    std::cout << "2" << std::endl;
    vuk_device_sf_resource.reset();
    std::cout << "3" << std::endl;
    context.reset();
    std::cout << "4" << std::endl;
    vkDestroySurfaceKHR(vkbInstance.instance, surface, nullptr);
    std::cout << "5" << std::endl;
    GlfwHelper::destroy_window_glfw(window);
    vkb::destroy_device(vkbDevice);
    std::cout << "6" << std::endl;
    vkb::destroy_instance(vkbInstance);
    std::cout << "Destroyed successfully" << std::endl;
}

void App::onResize(GLFWwindow *window, int width, int height) {

}

// Cleaning process
void App::cleanup() {
    context->wait_idle();
    /**
    for (auto& scene: Scenes)
    {
        for (auto& mesh: scene->Meshes)
        {
            for (auto& texture: mesh->Textures)
            {
                std::cout << "Freed texture" << std::endl;
                //texture.reset();
            }
        }
    }
    **/
    for (auto& buffer: this->scene.buffers) {
        if (buffer && buffer->vk_buffer) {
            buffer->vk_buffer.reset();
        }
    }

    for (auto& as: this->scene_acceleration_structures) {
        as.as.reset();
        as.buffer.reset();
    }
    std::cout << "Freed all textures" << std::endl;
    present_ready.reset();
    std::cout << "1" << std::endl;
    render_complete.reset();
    std::cout << "2" << std::endl;
    vuk_device_sf_resource.reset();
    std::cout << "3" << std::endl;
    context.reset();
    std::cout << "4" << std::endl;
    vkDestroySurfaceKHR(vkbInstance.instance, surface, nullptr);
    std::cout << "5" << std::endl;
    GlfwHelper::destroy_window_glfw(window);
    vkb::destroy_device(vkbDevice);
    std::cout << "6" << std::endl;
    vkb::destroy_instance(vkbInstance);
    std::cout << "Destroyed successfully" << std::endl;
}

// Sets up the app and begins the render loop
void App::setup() {
    glfwSetWindowSizeCallback(window,
                              [](GLFWwindow* window, int width, int height) {
    });
    {
        vuk::PipelineBaseCreateInfo pci;
        //pci.add_glsl(util::read_entire_file("../Shaders/rt.rgen"), "rt.rgen");
        //pci.add_glsl(util::read_entire_file("../Shaders/rt.rmiss"), "rt.rmiss");
        //pci.add_glsl(util::read_entire_file("C:/Users/Danny/CLionProjects/Dender/src/Shaders/rt.rchit"), "rt.rchit");
        pci.add_hit_group(vuk::HitGroup{
            .type = vuk::HitGroupType::eTriangles,
            .closest_hit = 2
        });
        context->create_named_pipeline("Path tracing", pci);
    }

    // Describe the mesh
    vuk::PipelineBaseCreateInfo pci;
    pci.add_glsl(util::read_entire_file("C:/Users/Danny/CLionProjects/Dender/src/Shaders/triangle.vert"), "vert");
    pci.add_glsl(util::read_entire_file("C:/Users/Danny/CLionProjects/Dender/src/Shaders/triangle.frag"), "frag");

    context->create_named_pipeline("triangle", pci);
}

void App::loop() {
    vuk::Compiler compiler;
    vuk::wait_for_futures_explicit(*vuk_allocator, compiler, *vuk_futures);
    vuk_futures->clear();
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        this->render(compiler);
    }
    // Process is done, break
    this->cleanup();

    //auto& aaa = vuk::create_buffer(;)
}

vuk::SingleSwapchainRenderBundle bundle;

void App::render(vuk::Compiler& compiler) {
    auto& device_frame_resource = vuk_device_sf_resource->get_next_frame();
    context->next_frame();
    vuk::Allocator frame_allocator(device_frame_resource);
    bundle = *vuk::acquire_one(*context, swapchain, (*present_ready)[context->get_frame_count() % 3],
                               (*render_complete)[context->get_frame_count() % 3]);

    /**
    std::vector<uint64_t> buffer_addresses;
    buffer_addresses.insert(buffer_addresses.end(), this->scene->m_bda.begin(), this->scene->m_bda.end());
    auto [bda_buf, bda_fut] = vuk::create_buffer(
            frame_allocator,
            vuk::MemoryUsage::eGPUonly,
            vuk::DomainFlagBits::eTransferOnGraphics,
            std::span(this->scene->m_bda)
    );
    auto ubo_buf = *bda_buf;
    **/

    // This struct will represent the view-projection transform used for the cube
    struct VP {
        glm::mat4 view;
        glm::mat4 proj;
    } vp{};
    // Fill the view matrix, looking a bit from top to the center
    vp.view = glm::lookAt(glm::vec3(0, 1.5, 3.5), glm::vec3(0), glm::vec3(0, 1, 0));
    // Fill the projection matrix, standard perspective matrix
    vp.proj = glm::perspective(glm::degrees(70.f), 1.f, 1.f, 100.f);
    vp.proj[1][1] *= -1;
    // Allocate and transfer view-projection transform
    auto [buboVP, uboVP_fut] = create_buffer(frame_allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnGraphics, std::span(&vp, 1));
    // since this memory is CPU visible (MemoryUsage::eCPUtoGPU), we don't need to wait for the future to complete
    auto uboVP = *buboVP;

    auto rendererName = vuk::Name("01_triangle");

    vuk::RenderGraph renderGraph("runner");
    renderGraph.attach_swapchain("_swp", swapchain);
    renderGraph.clear_image("_swp", rendererName, vuk::ClearColor{0.3f, 0.5f, 0.3f, 1.0f});

    vuk::RenderGraph rg("01");
    /*
    rg.add_pass({
        .resources = {"01_triangle"_image >> vuk::eColorWrite >> "01_triangle_final"},
        .execute = [](vuk::CommandBuffer& commandBuffer) {
            commandBuffer.set_viewport(0, vuk::Rect2D::framebuffer());
            commandBuffer.set_scissor(0, vuk::Rect2D::framebuffer());
            commandBuffer
                .set_rasterization({})
                .set_color_blend("01_triangle", {})
                .bind_graphics_pipeline("triangle")
                .draw(3, 1, 0, 0);
        }
    });
    */

    rg.attach_in("01_triangle",
                 std::move(vuk::Future{ std::make_shared<vuk::RenderGraph>(std::move(renderGraph)),
                                        rendererName}));

    auto gltf_scene = this->scene;
    rg.add_pass({
       .resources = {
               "01_triangle"_image >> vuk::eColorWrite >> "01_triangle_final"
       },
       .execute = [uboVP, gltf_scene](vuk::CommandBuffer& command_buffer) {

           command_buffer.set_viewport(0, vuk::Rect2D::framebuffer())
           .set_scissor(0, vuk::Rect2D::framebuffer())
           .set_rasterization({})
           .set_color_blend("01_triangle", {})
           .bind_buffer(0, 0, uboVP);
           auto* model = command_buffer.map_scratch_buffer<glm::mat4>(0, 1);
           *model = static_cast<glm::mat4>(glm::angleAxis(glm::radians(360.f), glm::vec3(0.f, 1.f, 0.f)));

           for (int i = 0; i < gltf_scene.meshes.size(); i++) {
               auto& mesh = gltf_scene.meshes[i];
			   vuk::VertexInputAttributeDescription vertex_attributes{};
			   vertex_attributes.format = mesh.positions.format;
			   vertex_attributes.offset = mesh.positions.offset;
			   vertex_attributes.binding = 0;
			   vertex_attributes.location = 0;

               command_buffer.bind_vertex_buffer(
                       0,
                       **mesh.positions.buffer->vk_buffer,
						std::span{&vertex_attributes, 1},
                       mesh.positions.stride
               )
               .bind_index_buffer(
                       **mesh.indices.buffer->vk_buffer,
                       mesh.indices.format
               )
               .bind_graphics_pipeline("triangle");
               command_buffer.draw_indexed(mesh.indices.count, 1, mesh.indices.first_index, 0, i);
           }
       }
    });

    auto fut = vuk::Future{std::make_unique<vuk::RenderGraph>(std::move(rg)), "01_triangle_final"};
    auto ptr = fut.get_render_graph();
    auto erg = *compiler.link(std::span{&ptr, 1}, {});
    auto result = *vuk::execute_submit(frame_allocator, std::move(erg), std::move(bundle));
    vuk::present_to_one(*context, std::move(result));
    if (++num_frames == 16)
    {
        auto new_time = glfwGetTime();
        auto delta = new_time - old_time;
        auto per_frame_time = delta / 16 * 1000;
        old_time = new_time;
        num_frames = 0;
        glfwSetWindowTitle(window, (APP_TITLE + std::string(" [") + std::to_string(per_frame_time) + " ms / " + std::to_string(1000 / per_frame_time) + " FPS]").c_str() );
    }
}

void App::LoadSceneFromFile(const std::string& path)
{
    /**
    // Load scene
    auto scene = new Scene(path);
    std::cout << "Running - Allocation" << std::endl;
    Scenes.push_back(scene);

    // Create buffers
    auto [vert_buf, vert_fut] = vuk::create_buffer(*vuk_allocator,
                       vuk::MemoryUsage::eGPUonly,
                       vuk::DomainFlagBits::eTransferOnGraphics,
                       std::span(scene->m_Vertices));
    m_VertexBuffer = std::move(vert_buf);
    vuk_futures->emplace_back(std::move(vert_fut));

    auto [ind_buf, ind_fut] = vuk::create_buffer(*vuk_allocator,
                                                   vuk::MemoryUsage::eGPUonly,
                                                   vuk::DomainFlagBits::eTransferOnGraphics,
                                                   std::span(scene->m_Indices));
    m_IndexBuffer = std::move(ind_buf);
    vuk_futures->emplace_back(std::move(ind_fut));


    auto [nrm_buf, nrm_fut] = vuk::create_buffer(*vuk_allocator,
                                                 vuk::MemoryUsage::eGPUonly,
                                                 vuk::DomainFlagBits::eTransferOnGraphics,
               m_buf);
                                  std::span(scene->m_Normals));
    m_IndexBuffer = std::move(nr    vuk_futures->emplace_back(std::move(nrm_fut));

    auto [uv_buf, uv_fut] = vuk::create_buffer(*vuk_allocator,
                                                 vuk::MemoryUsage::eGPUonly,
                                                 vuk::DomainFlagBits::eTransferOnGraphics,
                                                 std::span(scene->m_texCoords));
    **/

    std::filesystem::path filePath = std::filesystem::path{path};
    auto loaded_scene = this->gltf_loader.load_file(filePath, *this->vuk_allocator);
    //this->scene = static_cast<RenderScene>(this->gltf_loader.load_file(filePath, *this->vuk_allocator));
    this->scene = loaded_scene;

	std::vector<vuk::Future> futures;
	for (auto& buffer: this->scene.buffers) {
		futures.emplace_back(buffer->future);
	}
	this->vuk_futures->insert(this->vuk_futures->end(), futures.begin(), futures.end());

    // Build AS
    std::vector<AccelerationStructure::BlasInput> blas_inputs;
    for (auto& mesh: this->scene.meshes) {
        // Describe the mesh
        VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
        triangles.vertexFormat = static_cast<VkFormat>(mesh.positions.format);
        triangles.vertexData.deviceAddress = (*mesh.positions.buffer->vk_buffer)->device_address;
        triangles.vertexStride             = mesh.positions.stride;
        // Describe the index data
        triangles.indexType = static_cast<VkIndexType>(mesh.indices.format);
        triangles.indexData.deviceAddress = (*mesh.positions.buffer->vk_buffer)->device_address;
        // Indicate identity transform by setting transformData to null device pointer
        triangles.transformData = {};
        triangles.maxVertex = mesh.positions.count;

        // Identify the above data as containing opaque triangles
        VkAccelerationStructureGeometryKHR as_geometry { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        as_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        as_geometry.geometry.triangles = triangles;

        // Find sizes
        VkAccelerationStructureBuildRangeInfoKHR build_range_info;
        build_range_info.firstVertex = 0;
        build_range_info.primitiveCount = mesh.indices.count / 3;
        build_range_info.primitiveOffset = 0;
        build_range_info.transformOffset = 0;

        AccelerationStructure::BlasInput blas_input;
        blas_input.as_geometry.emplace_back(as_geometry);
        blas_input.as_build_offset_info.emplace_back(build_range_info);

        blas_inputs.emplace_back(blas_input);
    }
    AccelerationStructure::SceneAccelerationStructure rendergraph_as = this->acceleration_structure->build_blas(
            blas_inputs,
            VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
    );
    this->vuk_futures->emplace_back(
            vuk::Future(std::make_shared<vuk::RenderGraph>(std::move(rendergraph_as.graph)),
                    "blas_buffer+")
            );

    // Handle TLAS creation
    for (auto& mesh: this->scene.meshes) {

    }

    /**
    this->scene_acceleration_structures.insert(this->scene_acceleration_structures.begin(),
                                                rendergraph_as.acceleration_structures.begin(),
                                                rendergraph_as.acceleration_structures.end());
    **/
    for (auto& as: rendergraph_as.acceleration_structures) {
        this->scene_acceleration_structures.emplace_back(std::move(as));
    }
}