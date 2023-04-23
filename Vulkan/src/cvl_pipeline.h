#pragma once

#include <string>
#include <vector>

#include "cvl_device.h"

namespace cvl
{
	struct PipelineConfigInfo
	{
		PipelineConfigInfo() = default;
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		VkPipelineViewportStateCreateInfo viewport_info;
		VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
		VkPipelineRasterizationStateCreateInfo rasterization_info;
		VkPipelineMultisampleStateCreateInfo multisample_info;
		VkPipelineColorBlendAttachmentState color_blend_attachment;
		VkPipelineColorBlendStateCreateInfo color_blend_info;
		VkPipelineDepthStencilStateCreateInfo depth_stencil_info;
		std::vector<VkDynamicState> dynamic_state_enables;
		VkPipelineDynamicStateCreateInfo dynamic_state_info;
		VkPipelineLayout pipeline_layout = nullptr;
		VkRenderPass render_pass = nullptr;
		uint32_t subpass = 0;
	};

	class CvlPipeline
	{
	public:
		CvlPipeline
		(
			CvlDevice& device,
			const PipelineConfigInfo& config_info,
			const std::string& v_shader_fp, 
			const std::string& f_shader_fp
		);
		~CvlPipeline();

		CvlPipeline(const CvlPipeline&) = delete;
		CvlPipeline& operator=(const CvlPipeline&) = delete;

		void Bind(VkCommandBuffer command_buffer);

		static void DefaultPipelineConfigInfo(PipelineConfigInfo& config_info);
		static void SetGlslcFp(const std::string& fp) { _glslc_fp = fp; }

	private:
		static std::vector<char> ReadFile(const std::string& fp);
		static std::string CompileShader(const std::string& shader_fp);
		static std::string _glslc_fp;

		void CreateGraphicsPipeline(const std::string& v_shader_fp, const std::string& f_shader_fp, const PipelineConfigInfo& config_info);
		void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shader_module);
		CvlDevice& _cvl_device;
		VkPipeline _graphics_pipeline;
		VkShaderModule _v_shader_module;
		VkShaderModule _f_shader_module;

		std::vector<char> _code;
	};
}