#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace cvl 
{
	class CvlWindow 
	{
	public:
		CvlWindow(int width, int height, const std::string& name);
		~CvlWindow();
		CvlWindow(const CvlWindow&) = delete;
		CvlWindow& operator=(const CvlWindow&) = delete;
		CvlWindow(CvlWindow&&) = delete;
		CvlWindow& operator=(CvlWindow&&) = delete;
		
		void Init();
		void PollEvents() { glfwPollEvents(); }
		bool ShouldClose() { return glfwWindowShouldClose(_window); }
		VkExtent2D GetExtent() { return { static_cast<uint32_t>(_width), static_cast<uint32_t>(_height) }; }
		bool WasWindowResized() { return _framebuffer_resized; }
		void ResetWindowResizedFlag() { _framebuffer_resized = false; }

		GLFWwindow* GetGLFWwindow() { return _window; }
		void GetFramebufferSize(int* width, int* height) { glfwGetFramebufferSize(_window, width, height); }
		void CreateWindowSurface(VkInstance instance, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);

	private:
		static void FrambufferResizeCallback(GLFWwindow* window, int width, int height);

		GLFWwindow* _window;
		std::string _name;
		int _width, _height;
		bool _framebuffer_resized = false;
	};
}