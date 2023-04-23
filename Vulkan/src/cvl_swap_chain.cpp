#include "cvl_swap_chain.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#undef max

namespace cvl
{
	/* CvlSwapchain class */
	CvlSwapchain::CvlSwapchain(CvlDevice& device_ref, VkExtent2D extent)
		: _device(device_ref), _window_extent(extent)
	{
		Init();
	}
	
	CvlSwapchain::CvlSwapchain(CvlDevice& device_ref, VkExtent2D extent, std::shared_ptr<CvlSwapchain> previous)
		: _device(device_ref), _window_extent(extent), _old_swap_chain(previous)
	{
		Init();

		// Clean up old swap chain since it's no longer needed
		_old_swap_chain = nullptr;
	}

	void CvlSwapchain::Init()
	{
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateDepthResources();
		CreateFramebuffers();
		CreateSyncObjects();
	}

	CvlSwapchain::~CvlSwapchain()
	{
		for (auto image_view : _swap_chain_image_views)
		{
			vkDestroyImageView(_device.device(), image_view, nullptr);
		}
		_swap_chain_image_views.clear();

		if (_swap_chain != nullptr)
		{
			vkDestroySwapchainKHR(_device.device(), _swap_chain, nullptr);
			_swap_chain = nullptr;
		}

		for (int i = 0; i < _depth_images.size(); ++i)
		{
			vkDestroyImageView(_device.device(), _depth_image_views[i], nullptr);
			vkDestroyImage(_device.device(), _depth_images[i], nullptr);
			vkFreeMemory(_device.device(), _depth_image_memorys[i], nullptr);
		}

		for (auto framebuffer : _swap_chain_framebuffers)
		{
			vkDestroyFramebuffer(_device.device(), framebuffer, nullptr);
		}

		vkDestroyRenderPass(_device.device(), _render_pass, nullptr);

		// Cleanup synchronization objects
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vkDestroySemaphore(_device.device(), _render_finished_semaphores[i], nullptr);
			vkDestroySemaphore(_device.device(), _image_available_semaphores[i], nullptr);
			vkDestroyFence(_device.device(), _in_flight_fences[i], nullptr);
		}
	}

	VkResult CvlSwapchain::AquireNextImage(uint32_t* image_index)
	{
		vkWaitForFences
		(
			_device.device(),
			1,
			&_in_flight_fences[_current_frame],
			VK_TRUE,
			std::numeric_limits<uint64_t>::max()
		);
		VkResult result = vkAcquireNextImageKHR
		(
			_device.device(),
			_swap_chain,
			std::numeric_limits<uint64_t>::max(),
			_image_available_semaphores[_current_frame], // ! must be a not signaled semaphore
			VK_NULL_HANDLE,
			image_index
		);
		return result;
	}

	VkResult CvlSwapchain::SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* image_index)
	{
		if (_images_in_flight[*image_index] != VK_NULL_HANDLE)
		{
			vkWaitForFences(_device.device(), 1, &_images_in_flight[*image_index], VK_TRUE, UINT64_MAX);
		}
		_images_in_flight[*image_index] = _in_flight_fences[_current_frame];

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore wait_semaphores[] = { _image_available_semaphores[_current_frame] };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;

		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = buffers;

		VkSemaphore signal_semaphores[] = { _render_finished_semaphores[_current_frame] };
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_semaphores;

		vkResetFences(_device.device(), 1, &_in_flight_fences[_current_frame]);
		if (vkQueueSubmit(_device.GraphicsQueue(), 1, &submit_info, _in_flight_fences[_current_frame]) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlSwapchain] Failed to submit draw command buffer!");
		}

		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_semaphores;

		VkSwapchainKHR swap_chains[] = { _swap_chain };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swap_chains;

		present_info.pImageIndices = image_index;

		VkResult result = vkQueuePresentKHR(_device.PresentQueue(), &present_info);

		_current_frame = (_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

		return result;
	}

	void CvlSwapchain::CreateSwapChain()
	{
		SwapChainSupportDetails swap_chain_support = _device.GetSwapChainSupport();

		VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
		VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes);
		VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities);

		uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1; // recommended
		// maxImageCount == 0 means ther is no maximum
		if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
		{
			image_count = swap_chain_support.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = _device.surface();

		create_info.minImageCount = image_count;
		create_info.imageFormat = surface_format.format;
		create_info.imageColorSpace = surface_format.colorSpace;
		create_info.imageExtent = extent;
		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = _device.FindPhysicalQueueFamilies();
		uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

		if (indices.graphics_family != indices.present_family)
		{
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_family_indices;
		}
		else
		{
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;		// optional
			create_info.pQueueFamilyIndices = nullptr;	// optional
		}

		create_info.preTransform = swap_chain_support.capabilities.currentTransform;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		create_info.presentMode = present_mode;
		create_info.clipped = VK_TRUE;

		create_info.oldSwapchain = _old_swap_chain == nullptr ? VK_NULL_HANDLE : _old_swap_chain->_swap_chain;

		if (vkCreateSwapchainKHR(_device.device(), &create_info, nullptr, &_swap_chain) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlSwapchain] Failed to create swap chain!");
		}

		/*
			We only specified a minimum number of images in the swap chain, so the implementation is
			allowed to create a swap chain with more. That's why we'll first query the final number of
			images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
			retrieve the handles.
		*/
		vkGetSwapchainImagesKHR(_device.device(), _swap_chain, &image_count, nullptr);
		_swap_chain_images.resize(image_count);
		vkGetSwapchainImagesKHR(_device.device(), _swap_chain, &image_count, _swap_chain_images.data());

		_swap_chain_image_format = surface_format.format;
		_swap_chain_extent = extent;
	}

	void CvlSwapchain::CreateImageViews()
	{
		_swap_chain_image_views.resize(_swap_chain_images.size());
		for (size_t i = 0; i < _swap_chain_images.size(); ++i)
		{
			VkImageViewCreateInfo create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			create_info.image = _swap_chain_images[i];
			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = _swap_chain_image_format;
			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;
			if (vkCreateImageView(_device.device(), &create_info, nullptr, &_swap_chain_image_views[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("[CvlSwapchain] Failed to create image views!");
			}
		}
	}

	void CvlSwapchain::CreateRenderPass()
	{
		VkAttachmentDescription depth_attachment = {};
		depth_attachment.format = FindDepthFormat();
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref = {};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription color_attachment = {};
		color_attachment.format = GetSwapChainImageFormat();
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstSubpass = 0;
		dependency.dstStageMask = 
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };
		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(std::size(attachments));
		render_pass_info.pAttachments = attachments;
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		if (vkCreateRenderPass(_device.device(), &render_pass_info, nullptr, &_render_pass) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlSwapchain] Failed to create render pass!");
		}
	}

	void CvlSwapchain::CreateDepthResources()
	{
		VkFormat depth_format = FindDepthFormat();
		VkExtent2D swap_chain_extent = GetSwapChainExtent();

		_depth_images.resize(ImageCount());
		_depth_image_memorys.resize(ImageCount());
		_depth_image_views.resize(ImageCount());

		for (int i = 0; i < _depth_images.size(); ++i)
		{
			VkImageCreateInfo image_info = {};
			image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			image_info.imageType = VK_IMAGE_TYPE_2D;
			image_info.extent.width = swap_chain_extent.width;
			image_info.extent.height = swap_chain_extent.height;
			image_info.extent.depth = 1;
			image_info.mipLevels = 1;
			image_info.arrayLayers = 1;
			image_info.format = depth_format;
			image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
			image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			image_info.samples = VK_SAMPLE_COUNT_1_BIT;
			image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			image_info.flags = 0;

			_device.CreateImageWithInfo
			(
				image_info,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				_depth_images[i],
				_depth_image_memorys[i]
			);

			VkImageViewCreateInfo view_info = {};
			view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_info.image = _depth_images[i];
			view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view_info.format = depth_format;
			view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			view_info.subresourceRange.baseMipLevel = 0;
			view_info.subresourceRange.levelCount = 1;
			view_info.subresourceRange.baseArrayLayer = 0;
			view_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(_device.device(), &view_info, nullptr, &_depth_image_views[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("[CvlSwapchain] Failed to create texture image view!");
			}
		}
	}

	void CvlSwapchain::CreateFramebuffers()
	{
		_swap_chain_framebuffers.resize(ImageCount());
		for (size_t i = 0; i < ImageCount(); ++i)
		{
			VkImageView attachments[] = { _swap_chain_image_views[i], _depth_image_views[i] };

			VkExtent2D swap_chain_extent = GetSwapChainExtent();
			VkFramebufferCreateInfo framebuffer_info = {};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = _render_pass;
			framebuffer_info.attachmentCount = static_cast<uint32_t>(std::size(attachments));
			framebuffer_info.pAttachments = attachments;
			framebuffer_info.width = swap_chain_extent.width;
			framebuffer_info.height = swap_chain_extent.height;
			framebuffer_info.layers = 1;

			if (vkCreateFramebuffer(_device.device(), &framebuffer_info, nullptr, &_swap_chain_framebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("[CvlSwapchain] Failed to create framebuffer!");
			}
		}
	}

	void CvlSwapchain::CreateSyncObjects()
	{
		_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
		_images_in_flight.resize(ImageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info = {};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (vkCreateSemaphore(_device.device(), &semaphore_info, nullptr, &_image_available_semaphores[i])
				!= VK_SUCCESS ||
				vkCreateSemaphore(_device.device(), &semaphore_info, nullptr, &_render_finished_semaphores[i])
				!= VK_SUCCESS ||
				vkCreateFence(_device.device(), &fence_info, nullptr, &_in_flight_fences[i])
				!= VK_SUCCESS)
			{
				throw std::runtime_error("[CvlSwapchain] Failed to create synchronization objects for a frame!");
			}
		}
	}

	VkSurfaceFormatKHR CvlSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
	{
		for (const auto& available_format : available_formats)
		{
			if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
				available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return available_format;
			}
		}
		return available_formats[0];
	}

	VkPresentModeKHR CvlSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
	{
		for (const auto& available_present_mode : available_present_modes)
		{
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				std::cout << "[CvlSwapchain] Present mode: Mailbox" << std::endl;
				return available_present_mode;
			}
		}
		// VK_PRESENT_MODE_IMMEDIATE_KHR
		std::cout << "[CvlSwapchain] Present mode: V-Sync" << std::endl;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D CvlSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		VkExtent2D actual_extent = _window_extent;
		actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actual_extent;
	}

	VkFormat CvlSwapchain::FindDepthFormat()
	{
		std::vector<VkFormat> candidates =
		{
			VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
		};
		return _device.FindSupportedFormat
		(
			candidates,
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
	/* ~CvlSwapchain class */
}