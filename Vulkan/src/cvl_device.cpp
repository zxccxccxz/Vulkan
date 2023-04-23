#include "cvl_device.h"

#include <iostream>
#include <set>
#include <unordered_set>

#include <cstdint>
#include <limits>
#include <algorithm>

namespace cvl
{
	/* Callback */
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
		void* p_user_data)
	{
		if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			std::cerr << "[vl] " << p_callback_data->pMessage << std::endl;
		}
		return VK_FALSE;
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
		const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			return func(instance, p_create_info, p_allocator, p_debug_messenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* p_allocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, debug_messenger, p_allocator);
		}
	}

	/* CvlDevice class */

	CvlDevice::CvlDevice(CvlWindow& window) : _window(window)
	{
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPool();
	}

	CvlDevice::~CvlDevice()
	{
		vkDestroyCommandPool(_device, _command_pool, nullptr);
		vkDestroyDevice(_device, nullptr);
		if (_enable_validation_layers)
		{
			DestroyDebugUtilsMessengerEXT(_instance, _debug_messenger, nullptr);
		}
		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkDestroyInstance(_instance, nullptr);
	}

	/* Validation layers  */
	void CvlDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
	{
		create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = DebugCallback;
		create_info.pUserData = nullptr; // optional
	}

	void CvlDevice::SetupDebugMessenger()
	{
		if (!_enable_validation_layers) return;
		VkDebugUtilsMessengerCreateInfoEXT create_info;
		PopulateDebugMessengerCreateInfo(create_info);

		if (CreateDebugUtilsMessengerEXT(_instance, &create_info, nullptr, &_debug_messenger) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlDevice] Failed to set up debug messenger!");
		}
	}

	bool CvlDevice::CheckValidationLayerSupport()
	{
		uint32_t layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
		
		for (const char* layer_name : _validation_layers)
		{
			bool layer_found = false;

			for (const auto& layer_properties : available_layers)
			{
				if (strcmp(layer_name, layer_properties.layerName) == 0)
				{
					layer_found = true;
					break;
				}
			}

			if (!layer_found)
			{
				return false;
			}
		}
		return true;
	}
	/* ~Validation Layers */

	/* Devices */
	QueueFamilyIndices CvlDevice::FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		// Assign index to queue families that could be found
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
		uint32_t i = 0;
		for (const auto& queue_family : queue_families)
		{
			if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphics_family = i;
			}

			VkBool32 is_present_supported;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &is_present_supported);
			if (is_present_supported)
			{
				indices.present_family = i;
			}

			if (indices.IsComplete())
			{
				break;
			}
			++i;
		}
		return indices;
	}

	SwapChainSupportDetails CvlDevice::QuerySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details = {};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &format_count, nullptr);
		if (format_count != 0)
		{
			details.formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &format_count, details.formats.data());
		}

		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_mode_count, nullptr);
		if (present_mode_count != 0)
		{
			details.present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_mode_count, details.present_modes.data());
		}

		return details;
	}

	bool CvlDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions(_device_extensions.begin(), _device_extensions.end());

		for (const auto& extension : available_extensions)
		{
			required_extensions.erase(extension.extensionName);
		}

		return required_extensions.empty();
	}

	bool CvlDevice::IsDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = FindQueueFamilies(device);
		bool extensions_supported = CheckDeviceExtensionSupport(device);

		VkPhysicalDeviceFeatures device_features;
		vkGetPhysicalDeviceFeatures(device, &device_features);

		bool swap_chain_adequate = false;
		if (extensions_supported)
		{
			SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(device);
			swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
		}

		return indices.IsComplete() && extensions_supported && swap_chain_adequate && device_features.samplerAnisotropy;
	}

	void CvlDevice::PickPhysicalDevice()
	{
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(_instance, &device_count, nullptr);
		if (device_count == 0)
		{
			throw std::runtime_error("[CvlDevice] Failed to find GPUs with Vulkan support!");
		}
		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(_instance, &device_count, devices.data());
		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device))
			{
				_physical_device = device;
				break;
			}
		}
		if (_physical_device == VK_NULL_HANDLE)
		{
			throw std::runtime_error("[CvlDevice] Failed to find a suitable GPU!");
		}
		vkGetPhysicalDeviceProperties(_physical_device, &_physical_device_properties);
		std::cout << "[CvlDevice] Physical Device: " << _physical_device_properties.deviceName << std::endl;
	}
	
	// Logical Device
	void CvlDevice::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(_physical_device);

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

		float queue_priority = 1.0f;
		for (uint32_t queue_family : unique_queue_families)
		{
			VkDeviceQueueCreateInfo queue_create_info = {};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = queue_family;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;
			queue_create_infos.emplace_back(queue_create_info);
		}

		//
		VkPhysicalDeviceFeatures device_features = {};
		device_features.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.pEnabledFeatures = &device_features;
		create_info.enabledExtensionCount = static_cast<uint32_t>(_device_extensions.size());
		create_info.ppEnabledExtensionNames = _device_extensions.data();
		// By now there is no distinction between instance and device specific validation layers, so
		// This is optional
		if (_enable_validation_layers)
		{
			create_info.enabledLayerCount = static_cast<uint32_t>(_validation_layers.size());
			create_info.ppEnabledLayerNames = _validation_layers.data();
		}
		else
		{
			create_info.enabledLayerCount = 0;
		}

		if (vkCreateDevice(_physical_device, &create_info, nullptr, &_device) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlDevice] Failed to create logical device!");
		}

		vkGetDeviceQueue(_device, indices.graphics_family.value(), 0, &_graphics_queue);
		vkGetDeviceQueue(_device, indices.present_family.value(), 0, &_present_queue);
	}
	/* ~Devices */

	/* Surface */
	void CvlDevice::CreateSurface()
	{
		_window.CreateWindowSurface(_instance, nullptr, &_surface);	
	}

	VkSurfaceFormatKHR CvlDevice::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
	{
		for (const auto& available_format : available_formats)
		{
			if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return available_format;
			}
		}
		return available_formats[0];
	}

	VkPresentModeKHR cvl::CvlDevice::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
	{
		for (const auto& available_present_mode : available_present_modes)
		{
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return available_present_mode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D CvlDevice::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
#undef max
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			_window.GetFramebufferSize(&width, &height);

			VkExtent2D actual_extent =
			{
				static_cast<uint32_t>(width), static_cast<uint32_t>(height)
			};

			actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actual_extent;
		}
	}
	/* ~Surface */

	/* Instance */
	void CvlDevice::CreateInstance()
	{
		if (_enable_validation_layers && !CheckValidationLayerSupport())
		{
			throw std::runtime_error("[CvlDevice] Validation layers requested, but not available!");
		}

		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "First Vulkan App";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;

		if (!HasGlfwRequiredInstanceExtensions())
		{
			throw std::runtime_error("[CvlDevice] Missing required glfw extension!");
		}

		std::vector<const char*> extensions = GetRequiredExtensions();
		create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		create_info.ppEnabledExtensionNames = extensions.data();
		create_info.enabledLayerCount = 0;

		VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
		if (_enable_validation_layers)
		{
			create_info.enabledLayerCount = static_cast<uint32_t>(_validation_layers.size());
			create_info.ppEnabledLayerNames = _validation_layers.data();

			PopulateDebugMessengerCreateInfo(debug_create_info);
			create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
		}
		else
		{
			create_info.enabledLayerCount = 0;
			create_info.pNext = nullptr;
		}
		if (vkCreateInstance(&create_info, nullptr, &_instance) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlDevice] Failed to create instance!");
		}
		else
		{
			std::cout << "[CvlDevice] Instance created successfully\n";
		}
	}
	/* ~Instance */

	std::vector<const char*> CvlDevice::GetRequiredExtensions()
	{
		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions;
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
		if (_enable_validation_layers)
		{
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	bool CvlDevice::HasGlfwRequiredInstanceExtensions()
	{
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
		std::cout << "Supported extensions:\n";
		std::unordered_set<std::string> supported;
		for (const auto& extension : extensions)
		{
			std::cout << '\t' << extension.extensionName << '\n';
			supported.insert(extension.extensionName);
		}
		std::cout << "Required extensions:\n";
		auto required = GetRequiredExtensions();
		for (const auto& extension : required)
		{
			std::cout << '\t' << extension << '\n';
			if (supported.find(extension) == supported.end())
			{
				return false;
			}
		}
		return true;
	}

	uint32_t CvlDevice::FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties mem_properties;
		vkGetPhysicalDeviceMemoryProperties(_physical_device, &mem_properties);
		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i)
		{
			if ((type_filter & (1 << i)) &&
				(mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		throw std::runtime_error("[CvlDevice] Failed to find suitable memory type!");
	}

	VkFormat CvlDevice::FindSupportedFormat
	(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features
	)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(_physical_device, format, &properties);

			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}
		throw std::runtime_error("[CvlDevice] Failed to find supported format!");
	}

	/* Buffers */
	void CvlDevice::CreateBuffer
	(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& buffer_memory
	)
	{
		VkBufferCreateInfo buff_create_info = {};
		buff_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buff_create_info.size = size;
		buff_create_info.usage = usage;
		buff_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(_device, &buff_create_info, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlDevice] Failed to create vertex buffer!");
		}

		VkMemoryRequirements mem_requirements;
		vkGetBufferMemoryRequirements(_device, buffer, &mem_requirements);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_requirements.size;
		alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, properties);

		if (vkAllocateMemory(_device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlDevice] Failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(_device, buffer, buffer_memory, 0);
	}

	VkCommandBuffer CvlDevice::BeginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool = _command_pool;
		alloc_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(_device, &alloc_info, &command_buffer);

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer, &begin_info);
		return command_buffer;
	}

	void CvlDevice::EndSingleTimeCommands(VkCommandBuffer command_buffer)
	{
		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		vkQueueSubmit(_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(_graphics_queue);

		vkFreeCommandBuffers(_device, _command_pool, 1, &command_buffer);
	}

	void CvlDevice::CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
	{
		VkCommandBuffer command_buffer = BeginSingleTimeCommands();

		VkBufferCopy copy_region = {};
		copy_region.srcOffset = 0; // optional
		copy_region.dstOffset = 0; // optional
		copy_region.size = size;
		vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

		EndSingleTimeCommands(command_buffer);
	}

	void CvlDevice::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layer_count)
	{
		VkCommandBuffer command_buffer = BeginSingleTimeCommands();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layer_count;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		EndSingleTimeCommands(command_buffer);
	}

	void CvlDevice::CreateImageWithInfo(const VkImageCreateInfo& image_info, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory)
	{
		if (vkCreateImage(_device, &image_info, nullptr, &image) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlDevice] Failed to create image!");
		}

		VkMemoryRequirements mem_requirements;
		vkGetImageMemoryRequirements(_device, image, &mem_requirements);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_requirements.size;
		alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits, properties);

		if (vkAllocateMemory(_device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlDevice] Failed to allocate image memory!");
		}

		if (vkBindImageMemory(_device, image, image_memory, 0) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlDevice] Failed to bind image memory!");
		}
	}
	/* ~Buffers */

	/* Command Pool */
	void CvlDevice::CreateCommandPool()
	{
		QueueFamilyIndices queue_family_indices = FindQueueFamilies(_physical_device);
		VkCommandPoolCreateInfo pool_create_info = {};
		pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_create_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
		pool_create_info.flags = 
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(_device, &pool_create_info, nullptr, &_command_pool) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlDevice] Failed to create command pool!");
		}
	}
	/* ~Command Pool */


	/* ~CvlDevice class */
}