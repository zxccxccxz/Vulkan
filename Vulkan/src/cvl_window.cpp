#include "cvl_window.h"

#include <stdexcept>

namespace cvl
{

	CvlWindow::CvlWindow(int width, int height, const std::string& name)
		: _width(width), _height(height), _name(name)
	{
		Init();
	}

	CvlWindow::~CvlWindow()
	{
		if (_window)
		{
			glfwDestroyWindow(_window);
			glfwTerminate();
		}
	}

	void CvlWindow::Init()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		_window = glfwCreateWindow(_width, _height, _name.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(_window, this);
		glfwSetFramebufferSizeCallback(_window, FrambufferResizeCallback);
	}

	void CvlWindow::CreateWindowSurface(VkInstance instance, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface)
	{
		if (glfwCreateWindowSurface(instance, _window, allocator, surface) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlDevice] Failed to create window surface!");
		}
	}

	void CvlWindow::FrambufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto cvl_window = reinterpret_cast<CvlWindow*>(glfwGetWindowUserPointer(window));
		cvl_window->_framebuffer_resized = true;
		cvl_window->_width = width;
		cvl_window->_height = height;
	}

}