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
#include <glm/gtx/transform.hpp>
#include <glm/vec3.hpp>
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
    deviceBuilder = deviceBuilder
            .add_pNext(&vk12features)
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
    compute_queue = vkbDevice.get_queue(vkb::QueueType::compute).value();
    auto computer_queue_family_index = vkbDevice.get_queue_index(vkb::QueueType::compute).value();

    VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties = {};
    acceleration_structure_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    acceleration_structure_properties.pNext = nullptr;

    VkPhysicalDeviceProperties2 device_properties2 = {};
    device_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    device_properties2.pNext = &acceleration_structure_properties;

    // Query the device properties
    vkGetPhysicalDeviceProperties2(physical_device, &device_properties2);

    std::cout << "3" << std::endl;

	// Add function pointers
    vuk::ContextCreateParameters::FunctionPointers fps;
    fps.vkGetInstanceProcAddr = vkbInstance.fp_vkGetInstanceProcAddr;
    fps.vkGetDeviceProcAddr   = vkbInstance.fp_vkGetDeviceProcAddr;

    context.emplace(vuk::ContextCreateParameters{
            vkbInstance,
            device,
            physical_device,
            graphics_queue,
            graphics_queue_family_index,
            compute_queue,
            computer_queue_family_index,
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
	/**
    scene_acceleration_structure = new SceneAccelerationStructure(
            *this->vuk_allocator,
            acceleration_structure_properties);
	**/

    camera_obj = *(new camera);

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

void App::on_resize(GLFWwindow *window, int width, int height) {

    App& runner = *reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

    runner.vuk_allocator->deallocate(std::span{&this->swapchain->swapchain, 1});
    runner.vuk_allocator->deallocate(this->swapchain->image_views);
    runner.context->remove_swapchain(this->swapchain);
    runner.swapchain = this->context->add_swapchain(util::make_swapchain(this->vkbDevice, this->swapchain->swapchain));
    for (auto& iv: this->swapchain->image_views) {
        runner.context->set_name(iv.payload, "Swapchain ImageView");
    }

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
    /**
     * TODO: implement proper cleanup
    for (auto& buffer: this->scene.buffers) {
        if (buffer && buffer->vk_buffer) {
            buffer->vk_buffer.reset();
        }
    }
    **/

    /**
    for (auto& as: this->blas_acceleration_structures) {
        as.as.reset();
        //as.buffer.reset();
    }
    this->tlas_acceleration_structure.as.reset();
    this->tlas_acceleration_structure.buffer.reset();
    this->tlas_acceleration_structure.instances_buffer.reset();
    this->tlas_acceleration_structure.scratch_buffer.reset();
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

// Sets up the app and begins the render loop
void App::setup() {
    glfwSetWindowSizeCallback(window,
                              [](GLFWwindow* window, int width, int height) {
        App& runner = *reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

        runner.vuk_allocator->deallocate(std::span{&runner.swapchain->swapchain, 1});
        runner.vuk_allocator->deallocate(runner.swapchain->image_views);
        runner.context->remove_swapchain(runner.swapchain);
        runner.swapchain = runner.context->add_swapchain(util::make_swapchain(runner.vkbDevice, runner.swapchain->swapchain));
        for (auto& iv: runner.swapchain->image_views) {
            runner.context->set_name(iv.payload, "Swapchain ImageView");
        }
    });

    // Describe the mesh
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(util::read_entire_file( "C:/Users/Danny/CLionProjects/Dender/src/shaders/rt.rgen"),  "rt.rgen");
        pci.add_glsl(util::read_entire_file( "C:/Users/Danny/CLionProjects/Dender/src/shaders/rt.rmiss"), "rt.rmiss");
        pci.add_glsl(util::read_entire_file( "C:/Users/Danny/CLionProjects/Dender/src/shaders/rt.rchit"), "rt.rchit");
        // new for RT: a hit group is a collection of shaders identified by their index in the PipelineBaseCreateInfo
        // 2 => rt.rchit
        pci.add_hit_group(vuk::HitGroup{ .type = vuk::HitGroupType::eTriangles, .closest_hit = 2 });
        context->create_named_pipeline("raytracing", pci);
    }
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

    std::shared_ptr<vuk::RenderGraph> rg(std::make_shared<vuk::RenderGraph>("Dender"));
    rg->attach_swapchain("_swp", swapchain);
    rg->clear_image("_swp",
                    "example_target_image",
                    vuk::ClearColor{0.3f, 0.5f, 0.3f, 1.0f});

    vuk::Future cleared_image_to_render_into {std::move(rg), "example_target_image"};

    vuk::Future example_result = this->acquire_rendergraph(frame_allocator,
                                                           std::move(cleared_image_to_render_into));

    std::shared_ptr<vuk::RenderGraph> rendergraph_present (std::make_shared<vuk::RenderGraph>("presenter"));
    rendergraph_present->attach_in("_src", std::move(example_result));
    // we tell the rendergraph _src will be used for presetnation after the rendergraph
    rendergraph_present->release_for_present("_src");

    auto erg = *compiler.link(std::span{&rendergraph_present, 1}, {});

    auto result = *vuk::execute_submit(frame_allocator,
                                       std::move(erg),
                                       std::move(bundle));
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

vuk::Future App::acquire_rendergraph(vuk::Allocator &frame_allocator, vuk::Future target) {
    struct VP {
        glm::mat4 inv_view;
        glm::mat4 inv_proj;
    } vp;
    vp.inv_view = glm::lookAt(glm::vec3(0, 0, 2),
                              glm::vec3(0,0,0),
                              glm::vec3(0, 1, 0));
    vp.inv_proj = glm::perspective(glm::degrees(90.f), 1.f, 1.f, 1000.f);
    vp.inv_proj[1][1] *= -1;
    vp.inv_view = glm::inverse(vp.inv_view);
    vp.inv_proj = glm::inverse(vp.inv_proj);

    auto [buboVP, uboVP_fut] = vuk::create_buffer(frame_allocator,
                                                  vuk::MemoryUsage::eCPUtoGPU,
                                                  vuk::DomainFlagBits::eTransferOnGraphics,
                                                  std::span(&vp, 1));
    auto uboVP = *buboVP;

    vuk::RenderGraph raytrace_rg("12");
    raytrace_rg.attach_in("12_rt", std::move(target));
    //raytrace_rg.attach_buffer("tlas", *this->tlas_acceleration_structure.buffer);

    // TLAS update pass
    /**
    raytrace_rg.add_pass({
        .resources = {
                "tlas"_buffer >> vuk::eAccelerationStructureBuildWrite },
        .execute = [instance_buffer     = *this->tlas_acceleration_structure.instances_buffer,
                    tlas_scratch_buffer = *this->tlas_acceleration_structure.scratch_buffer,
                    tlas           = *this->tlas_acceleration_structure.as]
                (vuk::CommandBuffer& command_buffer) mutable {
            VkAccelerationStructureGeometryInstancesDataKHR instancesVk{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
            instancesVk.data.deviceAddress = instance_buffer.device_address;

            VkAccelerationStructureGeometryKHR topASGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
            topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            topASGeometry.geometry.instances = instancesVk;

            VkAccelerationStructureBuildGeometryInfoKHR tlas_build_info{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            tlas_build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
            tlas_build_info.geometryCount = 1;
            tlas_build_info.pGeometries = &topASGeometry;
            tlas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
            tlas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

            tlas_build_info.srcAccelerationStructure = tlas;
            tlas_build_info.dstAccelerationStructure = tlas;
            tlas_build_info.scratchData.deviceAddress = tlas_scratch_buffer.device_address;

            VkAccelerationStructureBuildRangeInfoKHR tlas_offset{ 1, 0, 0, 0 };
            const VkAccelerationStructureBuildRangeInfoKHR* ptlas_offset = &tlas_offset;
            command_buffer.build_acceleration_structures(1, &tlas_build_info, &ptlas_offset);
        }});
    **/

    // We use a eR8G8B8A8Unorm, as the swapchain is in sRGB which does not support storage use
    raytrace_rg.attach_image("12_rt_target",
                             vuk::ImageAttachment{
        .format = vuk::Format::eR8G8B8A8Unorm,
        .sample_count = vuk::SampleCountFlagBits::e1,
        .layer_count = 1
    });
    // This intermediate image is the same shape as the swapchain image
    raytrace_rg.inference_rule("12_rt_target",
                               vuk::same_shape_as("12_rt"));
    // Synchronize against the TLAS buffer to run this pass after the TLAS update has completed
    raytrace_rg.add_pass({
        .resources = { "12_rt_target"_image >> vuk::eRayTracingWrite},
                       .execute = [uboVP](vuk::CommandBuffer& command_buffer) {

            command_buffer
            .bind_image(0, 1, "12_rt_target")
            .bind_buffer(0, 2, uboVP)
            .bind_ray_tracing_pipeline("raytracing");
            /**
            // Launch one ray per pixel in the intermediate image
            auto extent = command_buffer.get_resource_image_attachment("12_rt_target")->extent;
            command_buffer.trace_rays(extent.extent.width, extent.extent.height, 1);
            **/
        }});
    // Perform a blit of the intermediate image onto the swapchain
    // (this will also do the non-linear encoding for us, although we lost some precision when
    // we rendered into Unorm)
    raytrace_rg.add_pass({
        .resources = { "12_rt_target+"_image >> vuk::eTransferRead,
                       "12_rt"_image >> vuk::eTransferWrite >> "12_rt_final" },
                       .execute = [](vuk::CommandBuffer& command_buffer) {
            vuk::ImageBlit blit;
            blit.srcSubresource.aspectMask = vuk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.srcSubresource.mipLevel = 0;
            blit.dstSubresource = blit.srcSubresource;
            auto extent = command_buffer.get_resource_image_attachment("12_rt_target+")->extent;
            blit.srcOffsets[1] = vuk::Offset3D{ static_cast<int>(extent.extent.width), static_cast<int>(extent.extent.height), 1 };
            blit.dstOffsets[1] = blit.srcOffsets[1];
            command_buffer.blit_image("12_rt_target+", "12_rt", blit, vuk::Filter::eNearest);
        }});

    return vuk::Future { std::make_unique<vuk::RenderGraph>(std::move(raytrace_rg)), "12_rt_final" };
}

void App::LoadSceneFromFile(const std::string& path)
{
    std::filesystem::path filePath = std::filesystem::path{path};
    auto loaded_scene = this->gltf_loader.load_file(filePath, *this->vuk_allocator);
    //this->scene = static_cast<RenderScene>(this->gltf_loader.load_file(filePath, *this->vuk_allocator));
    this->scene = loaded_scene;

	std::vector<vuk::Future> futures;
	futures.reserve(this->scene.buffer_handler.get_handles().size());
	for (Handle<Asset::Buffer> buffer: this->scene.buffer_handler.get_handles()) {
		if (buffer.is_valid())
			futures.emplace_back(this->scene.buffer_handler.get(buffer).unwrap()->future);
	}
	this->vuk_futures->insert(this->vuk_futures->end(), futures.begin(), futures.end());
}