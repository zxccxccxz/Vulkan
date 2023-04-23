#pragma once

#include "cvl_device.h"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <memory>

namespace cvl
{
	class CvlSwapchain
	{
	public:
		CvlSwapchain(CvlDevice& device_ref, VkExtent2D extent);
		CvlSwapchain(CvlDevice& device_ref, VkExtent2D extent, std::shared_ptr<CvlSwapchain> previous);
		~CvlSwapchain();

		CvlSwapchain(const CvlSwapchain&) = delete;
		CvlSwapchain& operator=(const CvlSwapchain&) = delete;

		VkFramebuffer GetFramebuffer(int index) { return _swap_chain_framebuffers[index]; }
		VkRenderPass GetRenderPass() { return _render_pass; }
		VkImageView GetImageView(int index) { return _swap_chain_image_views[index]; }
		size_t ImageCount() { return _swap_chain_images.size(); }
		VkFormat GetSwapChainImageFormat() { return _swap_chain_image_format; }
		VkExtent2D GetSwapChainExtent() { return _swap_chain_extent; }
		uint32_t width() { return _swap_chain_extent.width; }
		uint32_t height() { return _swap_chain_extent.height; }

		float ExtentAspectRatio()
		{
			return static_cast<float>(_swap_chain_extent.width) / static_cast<float>(_swap_chain_extent.height);
		}
		VkFormat FindDepthFormat();

		VkResult AquireNextImage(uint32_t* image_index);
		VkResult SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* image_index);

		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	private:
		void Init();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateDepthResources();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateSyncObjects();

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		VkFormat _swap_chain_image_format;
		VkExtent2D _swap_chain_extent;
		
		std::vector<VkFramebuffer> _swap_chain_framebuffers;
		VkRenderPass _render_pass;

		std::vector<VkImage> _depth_images;
		std::vector<VkDeviceMemory> _depth_image_memorys;
		std::vector<VkImageView> _depth_image_views;
		std::vector<VkImage> _swap_chain_images;
		std::vector<VkImageView> _swap_chain_image_views;

		CvlDevice& _device;
		VkExtent2D _window_extent;

		VkSwapchainKHR _swap_chain;
		std::shared_ptr<CvlSwapchain> _old_swap_chain;

		std::vector<VkSemaphore> _image_available_semaphores;
		std::vector<VkSemaphore> _render_finished_semaphores;
		std::vector<VkFence> _in_flight_fences;
		std::vector<VkFence> _images_in_flight;
		size_t _current_frame = 0;
	};

}