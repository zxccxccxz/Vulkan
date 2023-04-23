#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <optional>

#include "cvl_window.h"

namespace cvl
{
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;

		bool IsComplete()
		{
			return graphics_family.has_value() && present_family.has_value();
		}
	};

	class CvlDevice
	{
	public:
#ifdef NDEBUG
		static constexpr bool _enable_validation_layers = false;
#else
		static constexpr bool _enable_validation_layers = true;
#endif
		CvlDevice(CvlWindow& window);
		~CvlDevice();

		CvlDevice(const CvlDevice&) = delete;
		CvlDevice& operator=(const CvlDevice&) = delete;
		CvlDevice(CvlDevice&&) = delete;
		CvlDevice& operator=(CvlDevice&&) = delete;

		VkDevice device() { return _device; }
		VkSurfaceKHR surface() { return _surface; }
		VkQueue GraphicsQueue() { return _graphics_queue; }
		VkQueue PresentQueue() { return _present_queue; }
		VkCommandPool GetCommandPool() { return _command_pool;  }

		SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(_physical_device); }
		QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(_physical_device); }

		uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
		VkFormat FindSupportedFormat
		(
			const std::vector<VkFormat>& candidates,
			VkImageTiling tiling,
			VkFormatFeatureFlags features
		);

		/* Buffers */
		void CreateBuffer
		(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& buffer_memory
		);
		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer command_buffer);
		void CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count);

		void CreateImageWithInfo
		(
			const VkImageCreateInfo& image_info,
			VkMemoryPropertyFlags properties,
			VkImage& image,
			VkDeviceMemory& image_memory
		);

	private:
		VkInstance _instance;
		CvlWindow& _window;

		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPool();

		/* Devices */
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		bool IsDeviceSuitable(VkPhysicalDevice device);

		/* Surface */
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		/* Validation layers */
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
		bool CheckValidationLayerSupport();
		std::vector<const char*> GetRequiredExtensions();
		bool HasGlfwRequiredInstanceExtensions();

		/* Validation layers */
		VkDebugUtilsMessengerEXT _debug_messenger;
		std::vector<const char*> _validation_layers =
		{
			"VK_LAYER_KHRONOS_validation", 
			//"VK_LAYER_LUNARG_standard_validation"
		};

		/* Devices */
		VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties _physical_device_properties;
		VkQueue _graphics_queue;
		VkQueue _present_queue;
		std::vector<const char*> _device_extensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		
		// Logical Device
		VkDevice _device;

		/* Surface */
		VkSurfaceKHR _surface;

		/* Command Pool */
		VkCommandPool _command_pool;
	};
}