#include "Application.h"

#include <stdexcept>

namespace cvl
{
	Application::Application()
		: 
		_cvl_window(std::make_unique<CvlWindow>(WIDTH, HEIGHT, "Vulkan")),
		_cvl_device(std::make_unique<CvlDevice>(*_cvl_window))
	{
		LoadModels();
		CreatePipelineLayout();
		RecreateSwapchain();
		CreateCommandBuffers();
	}

	Application::~Application()
	{
		vkDestroyPipelineLayout(_cvl_device->device(), _pipeline_layout, nullptr);
	}

	void Application::Run()
	{
		while (!_cvl_window->ShouldClose())
		{
			_cvl_window->PollEvents();
			DrawFrame();
		}

		vkDeviceWaitIdle(_cvl_device->device());
	}

	void Application::LoadModels()
	{
		std::vector<CvlModel::Vertex> vertices =
		{
			{{ 0.0f, -0.5f}, { 1.0f, 0.0f, 0.0f }},
			{{ 0.5f,  0.5f}, { 0.0f, 1.0f, 0.0f }},
			{{-0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f }}
		};

		_cvl_model = std::make_unique<CvlModel>(*_cvl_device, vertices);
	}

	void Application::CreatePipelineLayout()
	{
		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 0;
		pipeline_layout_info.pSetLayouts = nullptr;
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(_cvl_device->device(), &pipeline_layout_info, nullptr, &_pipeline_layout) != VK_SUCCESS)
		{
			throw std::runtime_error("[Application] Failed to create pipeline layout!");
		}
	}

	void Application::CreatePipeline()
	{
		assert(_cvl_swap_chain != nullptr && "Cannot create pipeline before swap chain");
		assert(_pipeline_layout != nullptr && "Cannot create pipeline before pipeline layout");
		PipelineConfigInfo pipeline_config = {};
		CvlPipeline::DefaultPipelineConfigInfo(pipeline_config);
		pipeline_config.render_pass = _cvl_swap_chain->GetRenderPass();
		pipeline_config.pipeline_layout = _pipeline_layout;
		_cvl_pipeline = std::make_unique<CvlPipeline>(*_cvl_device, pipeline_config, "src\\shaders\\shader.vert", "src\\shaders\\shader.frag");
	}

	void Application::RecreateSwapchain()
	{
		auto extent = _cvl_window->GetExtent();
		while (extent.width == 0 || extent.height == 0)
		{
			extent = _cvl_window->GetExtent();
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(_cvl_device->device());
		if (_cvl_swap_chain == nullptr)
		{
			_cvl_swap_chain = std::make_unique<CvlSwapchain>(*_cvl_device, extent);
		}
		else
		{
			_cvl_swap_chain = std::make_unique<CvlSwapchain>(*_cvl_device, extent, std::move(_cvl_swap_chain));
			if (_cvl_swap_chain->ImageCount() != _command_buffers.size())
			{
				FreeCommandBuffers();
				CreateCommandBuffers();
			}
		}
		// todo: If render pass compatible do nothing else
		CreatePipeline();
	}

	void Application::RecordCommandBuffer(int image_index)
	{
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(_command_buffers[image_index], &begin_info) != VK_SUCCESS)
		{
			throw std::runtime_error("[Application] Failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = _cvl_swap_chain->GetRenderPass();
		render_pass_info.framebuffer = _cvl_swap_chain->GetFramebuffer(image_index);

		render_pass_info.renderArea.offset = { 0, 0 };
		render_pass_info.renderArea.extent = _cvl_swap_chain->GetSwapChainExtent();

		VkClearValue clear_values[2];
		clear_values[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
		// clear_values[0].depthStencil = ? ; // this is incorrect, because | VkCLearValue is union
		// In render pass we structured our attachment so that index 0 is the color attachment and index 1 is color attachment
		clear_values[1].depthStencil = { 1.0f, 0 };
		render_pass_info.clearValueCount = static_cast<uint32_t>(std::size(clear_values));
		render_pass_info.pClearValues = clear_values;

		vkCmdBeginRenderPass(_command_buffers[image_index], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(_cvl_swap_chain->GetSwapChainExtent().width);
		viewport.height = static_cast<float>(_cvl_swap_chain->GetSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, _cvl_swap_chain->GetSwapChainExtent() };
		vkCmdSetViewport(_command_buffers[image_index], 0, 1, &viewport);
		vkCmdSetScissor(_command_buffers[image_index], 0, 1, &scissor);

		_cvl_pipeline->Bind(_command_buffers[image_index]);
		//vkCmdDraw(_command_buffers[i], 3, 1, 0, 0); // 3 vertices 1 instance(for multiple copies)
		_cvl_model->Bind(_command_buffers[image_index]);
		_cvl_model->Draw(_command_buffers[image_index]);

		vkCmdEndRenderPass(_command_buffers[image_index]);

		if (vkEndCommandBuffer(_command_buffers[image_index]) != VK_SUCCESS)
		{
			throw std::runtime_error("[Application] Failed to record command buffer!");
		}
	}

	void Application::CreateCommandBuffers()
	{
		_command_buffers.resize(_cvl_swap_chain->ImageCount());

		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool = _cvl_device->GetCommandPool();
		alloc_info.commandBufferCount = static_cast<uint32_t>(_command_buffers.size());

		if (vkAllocateCommandBuffers(_cvl_device->device(), &alloc_info, _command_buffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("[Application] Failed to allocate command buffers!");
		}
	}

	void Application::FreeCommandBuffers()
	{
		vkFreeCommandBuffers(_cvl_device->device(), _cvl_device->GetCommandPool(), static_cast<uint32_t>(_command_buffers.size()), _command_buffers.data());
		_command_buffers.clear();
	}

	void Application::DrawFrame()
	{
		uint32_t image_index;
		VkResult result = _cvl_swap_chain->AquireNextImage(&image_index);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();
			return;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("[Application] Failed to acquire swap chain image!");
		}

		RecordCommandBuffer(image_index);
		result = _cvl_swap_chain->SubmitCommandBuffers(&_command_buffers[image_index], &image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _cvl_window->WasWindowResized())
		{
			_cvl_window->ResetWindowResizedFlag();
			RecreateSwapchain();
			return;
		}
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("[Application] Failed to present swap chain image!");
		}
	}
}