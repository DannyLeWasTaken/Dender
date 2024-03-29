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
#include "./Assets/fast_gltf_loader.hpp"
#include "./camera.hpp"

class Scene;
class GltfScene;

class App {
public:

    // CONFIGURATION
    const std::string APP_TITLE = "Danny's Renderer";

    // VARIABLES
    VkDevice                                        device;
    VkPhysicalDevice                                physical_device;
    VkQueue                                         graphics_queue;
    VkQueue                                         transfer_queue;
    VkQueue                                         compute_queue;
    std::optional<vuk::Context>                     context;
    std::optional<vuk::DeviceSuperFrameResource>    vuk_device_sf_resource;
    std::optional<vuk::Allocator>                   vuk_allocator;
    vuk::SwapchainRef                               swapchain;
    vuk::Unique<std::array<VkSemaphore, 3>>         present_ready;
    vuk::Unique<std::array<VkSemaphore, 3>>         render_complete;


    std::shared_ptr<std::vector<vuk::Future>> vuk_futures;

    GLFWwindow*  window;
    VkSurfaceKHR surface;
    vkb::Instance vkbInstance;
    vkb::Device   vkbDevice;

    Asset::Scene scene;
    camera camera_obj;

    // FUNCTIONS
    App();
    ~App();
    void setup();
    void cleanup();
    void on_resize(GLFWwindow* window, int width, int height);
    void loop();
    void render(vuk::Compiler& command_buffer);
    vuk::Future acquire_rendergraph(vuk::Allocator& frame_allocator, vuk::Future future);


    void LoadSceneFromFile(const std::string& path);

protected:

    vuk::Unique<vuk::Buffer> m_VertexBuffer;
    vuk::Unique<vuk::Buffer> m_IndexBuffer;
    vuk::Unique<vuk::Buffer> m_NormalBuffer;

private:
    int num_frames = 0;
    double old_time;
	fast_gltf_loader gltf_loader;
};


#endif //DENDER_APP_HPP
