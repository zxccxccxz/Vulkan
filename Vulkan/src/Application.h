#pragma once

#include "cvl_pipeline.h"
#include "cvl_window.h"
#include "cvl_device.h"
#include "cvl_swap_chain.h"
#include "cvl_model.h"

#include <memory>
#include <vector>

namespace cvl
{

	class Application
	{
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		Application();
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		void Run();

	private:
		void LoadModels();
		void CreatePipelineLayout();
		void CreatePipeline();
		void CreateCommandBuffers();
		void FreeCommandBuffers();
		void DrawFrame();
		void RecreateSwapchain();
		void RecordCommandBuffer(int image_index);

		std::unique_ptr<CvlWindow> _cvl_window;
		std::unique_ptr<CvlDevice> _cvl_device;
		std::unique_ptr<CvlSwapchain> _cvl_swap_chain;
		std::unique_ptr<CvlPipeline> _cvl_pipeline;
		VkPipelineLayout _pipeline_layout;
		std::vector<VkCommandBuffer> _command_buffers;

		std::unique_ptr<CvlModel> _cvl_model;
	};
}