//
// Created by Danny on 2023-01-13.
//

#include "App.hpp"
#include "GlfwHelper.hpp"
#include "utils.hpp"
#include <vulkan/vulkan.h>
#include <iostream>
#include <vuk/Buffer.hpp>
//#include <glm/glm.hpp>

// Initialization of everything
App::App() {
    vkb::InstanceBuilder builder;
    builder.request_validation_layers(true)
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

    auto phys_ret = selector.select();
    vkb::PhysicalDevice vkbPhysicalDevice;
    if (!phys_ret) {
        throw std::runtime_error("Device does not support one or more of the following extensions:\n"
                                 " - VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME\n"
                                 " - VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME\n"
                                 " - VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME\n"
                                 " - VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME\n"
                                 "Common cause: Non-hardware accelerated ray tracing GPU");
    }
    vkbPhysicalDevice = phys_ret.value();
    physicalDevice = vkbPhysicalDevice.physical_device;

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
    if (!dev_ret)
        throw std::runtime_error("Couldn't create device");

    vkbDevice = dev_ret.value();
    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    auto graphics_queue_family_index = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
    auto transfer_queue_family_index = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();
    device = vkbDevice.device;

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
        physicalDevice,
        graphicsQueue,
        graphics_queue_family_index,
        VK_NULL_HANDLE,
        VK_QUEUE_FAMILY_IGNORED,
        transferQueue,
        transfer_queue_family_index,
    });

    const unsigned num_inflight_frames = 3;
    vukDeviceSfResource.emplace(*context, num_inflight_frames);
    vukAllocator.emplace(*vukDeviceSfResource);
    swapchain = context->add_swapchain(util::make_swapchain(vkbDevice, {}));
    presentReady = vuk::Unique<std::array<VkSemaphore, 3>>(*vukAllocator);
    renderComplete = vuk::Unique<std::array<VkSemaphore, 3>>(*vukAllocator);

    vukAllocator->allocate_semaphores(*presentReady);
    vukAllocator->allocate_semaphores(*renderComplete);
}

// Destruction of all
App::~App() {
    presentReady.reset();
    renderComplete.reset();
    vukDeviceSfResource.reset();
    context.reset();
    vkDestroySurfaceKHR(vkbInstance.instance, surface, nullptr);
    GlfwHelper::destroy_window_glfw(window);
    vkb::destroy_device(vkbDevice);
    vkb::destroy_instance(vkbInstance);
}

void App::onResize(GLFWwindow *window, int width, int height) {

}

// Cleaning process
void App::cleanup() {
    context->wait_idle();
}

// Sets up the app and begins the render loop
void App::setup() {
    glfwSetWindowSizeCallback(window,
                              [](GLFWwindow* window, int width, int height) {

    });



    this->loop();
}

void App::loop() {
    vuk::Compiler compiler;
    vuk::wait_for_futures_explicit(*vukAllocator, compiler, vukFutures);
    vukFutures.clear();
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        this->render();
    }
    // Process is done, break
    this->cleanup();
    delete this;
}

void App::render() {
    auto& vukDeviceFrameResource = vukDeviceSfResource->get_next_frame();
    context->next_frame();
    vuk::Allocator frameAllocator(vukDeviceFrameResource);
    vuk::RenderGraph rg("RayTraceGraph");

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

void App::LoadFromScene(std::string path)
{
    // Load scene
    //const aiScene* scene = util::loadFromFile(path);
    // Buffer the vertices
}