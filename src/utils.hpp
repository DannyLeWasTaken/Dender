//
// Created by Danny on 2023-01-13.
//

#ifndef DENDER_UTILS_HPP
#define DENDER_UTILS_HPP


#include <optional>
#include <vuk/Swapchain.hpp>
#include <VkBootstrap.h>

#include <iostream>
#include <sstream>
#include <fstream>

namespace util
{
    inline vuk::Swapchain make_swapchain(const vkb::Device& vkbdevice, std::optional<VkSwapchainKHR> old_swapchain) {
		vkb::SwapchainBuilder swb(vkbdevice);
		swb.set_desired_format(vuk::SurfaceFormatKHR{ vuk::Format::eR8G8B8A8Srgb, vuk::ColorSpaceKHR::eSrgbNonlinear });
		swb.add_fallback_format(vuk::SurfaceFormatKHR{ vuk::Format::eB8G8R8A8Srgb, vuk::ColorSpaceKHR::eSrgbNonlinear });
		swb.set_desired_present_mode((VkPresentModeKHR)vuk::PresentModeKHR::eImmediate);
		swb.set_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		if (old_swapchain) {
			swb.set_old_swapchain(*old_swapchain);
		}
		auto vkswapchain = swb.build();

		vuk::Swapchain sw{};
		auto images = *vkswapchain->get_images();
		auto views = *vkswapchain->get_image_views();

		for (auto& i : images) {
			sw.images.push_back(
                    vuk::Image{i, nullptr}
                    );
		}
		for (auto& i : views) {
			sw.image_views.emplace_back();
			sw.image_views.back().payload = i;
		}
		sw.extent = vuk::Extent2D{ vkswapchain->extent.width, vkswapchain->extent.height };
		sw.format = vuk::Format(vkswapchain->image_format);
		sw.surface = vkbdevice.surface;
		sw.swapchain = vkswapchain->swapchain;
		return sw;
	}

    inline std::string read_entire_file(const std::string& path)
    {
        std::ostringstream buf;
        std::ifstream input(path.c_str());
        std::cout << path << std::endl;
        assert(input);
        buf << input.rdbuf();
        return buf.str();
    }
}

#endif //DENDER_UTILS_HPP
