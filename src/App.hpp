//
// Created by Danny on 2023-01-13.
//

#ifndef DENDER_APP_HPP
#define DENDER_APP_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <optional>

#include <vuk/Allocator.hpp>
#include <vuk/AllocatorHelpers.hpp>
#include <vuk/CommandBuffer.hpp>
#include "vuk/Context.hpp"
#include "vuk/Partials.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/SampledImage.hpp"
#include "vuk/resources/DeviceFrameResource.hpp"
#include "./Structs/Assets.h"

class Scene;
class GltfScene;

class App {
public:

    // CONFIGURATION
    const std::string APP_TITLE = "Danny's Renderer";

    // VARIABLES
    VkDevice                                        device;
    VkPhysicalDevice                                physicalDevice;
    VkQueue                                         graphicsQueue;
    VkQueue                                         transferQueue;
    std::optional<vuk::Context>                     context;
    std::optional<vuk::DeviceSuperFrameResource>    vukDeviceSfResource;
    std::optional<vuk::Allocator>                   vukAllocator;
    vuk::SwapchainRef                               swapchain;
    vuk::Unique<std::array<VkSemaphore, 3>>         presentReady;
    vuk::Unique<std::array<VkSemaphore, 3>>         renderComplete;


    std::shared_ptr<std::vector<vuk::Future>> vukFutures;

    GLFWwindow*  window;
    VkSurfaceKHR surface;

    vkb::Instance vkbInstance;
    vkb::Device   vkbDevice;

    GltfScene* scene;

    // FUNCTIONS
    App();
    ~App();
    void setup();
    void cleanup();
    void onResize(GLFWwindow* window, int width, int height);
    void loop();
    void render(vuk::Compiler& command_buffer);

    void LoadSceneFromFile(std::string path);

protected:

    vuk::Unique<vuk::Buffer> m_VertexBuffer;
    vuk::Unique<vuk::Buffer> m_IndexBuffer;
    vuk::Unique<vuk::Buffer> m_NormalBuffer;

private:
    int num_frames = 0;
    double old_time;

    void PrimitiveToGeometryVk(const Primitive *mesh);
};


#endif //DENDER_APP_HPP
