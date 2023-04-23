#include "cvl_pipeline.h"

#include "cvl_model.h"

#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <cassert>

namespace cvl
{
	std::string CvlPipeline::_glslc_fp = "C:\\VulkanSDK\\1.3.239.0\\Bin\\glslc.exe";
	std::string CvlPipeline::CompileShader(const std::string& shader_fp)
	{
		// Windows only
		std::string cmd = _glslc_fp + " " + shader_fp + " -o " + shader_fp + ".spv";
		std::cout << "[CvlPipeline] " << cmd << '\n';
		system(cmd.c_str());
		return shader_fp + ".spv";
	}

	std::vector<char> CvlPipeline::ReadFile(const std::string& fp)
	{
		std::ifstream ifs(fp, std::ios::ate | std::ios::binary);

		if (!ifs.is_open())
		{
			throw std::runtime_error("[CvlPipeline] Failed to open file: " + fp);
		}

		size_t fsize = static_cast<size_t>(ifs.tellg());
		std::vector<char> buffer(fsize);
		ifs.seekg(0);
		ifs.read(buffer.data(), fsize);

		ifs.close();
		return buffer;
	}

	void CvlPipeline::DefaultPipelineConfigInfo(PipelineConfigInfo& config_info)
	{
		config_info.input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		config_info.input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		config_info.input_assembly_info.primitiveRestartEnable = VK_FALSE;

		config_info.viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		config_info.viewport_info.viewportCount = 1;
		config_info.viewport_info.pViewports = nullptr;
		config_info.viewport_info.scissorCount = 1;
		config_info.viewport_info.pScissors = nullptr;

		config_info.rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		config_info.rasterization_info.depthClampEnable = VK_FALSE;
		config_info.rasterization_info.rasterizerDiscardEnable = VK_FALSE;
		config_info.rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
		config_info.rasterization_info.lineWidth = 1.0f;
		config_info.rasterization_info.cullMode = VK_CULL_MODE_NONE; // BACK_BIT
		config_info.rasterization_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		config_info.rasterization_info.depthBiasEnable = VK_FALSE;
		config_info.rasterization_info.depthBiasConstantFactor = 0.0f;	// optional
		config_info.rasterization_info.depthBiasClamp = 0.0f;			// optional
		config_info.rasterization_info.depthBiasSlopeFactor = 0.0f;		// optional

		config_info.multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		config_info.multisample_info.sampleShadingEnable = VK_FALSE;
		config_info.multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		config_info.multisample_info.minSampleShading = 1.0f; // optional
		config_info.multisample_info.pSampleMask = nullptr; // optional
		config_info.multisample_info.alphaToCoverageEnable = VK_FALSE; // optional
		config_info.multisample_info.alphaToOneEnable = VK_FALSE; // optional
		
		config_info.color_blend_attachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		config_info.color_blend_attachment.blendEnable = VK_FALSE;
		config_info.color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;	// optional
		config_info.color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;	// optional
		config_info.color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;				// optional
		config_info.color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;	// optional
		config_info.color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;	// optional
		config_info.color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;				// optional

		config_info.color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		config_info.color_blend_info.logicOpEnable = VK_FALSE;
		config_info.color_blend_info.logicOp = VK_LOGIC_OP_COPY;	// optional
		config_info.color_blend_info.attachmentCount = 1;
		config_info.color_blend_info.pAttachments = &config_info.color_blend_attachment;
		config_info.color_blend_info.blendConstants[0] = 0.0f;	// optional
		config_info.color_blend_info.blendConstants[1] = 0.0f;	// optional
		config_info.color_blend_info.blendConstants[2] = 0.0f;	// optional
		config_info.color_blend_info.blendConstants[3] = 0.0f;	// optional

		config_info.depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		config_info.depth_stencil_info.depthTestEnable = VK_TRUE;
		config_info.depth_stencil_info.depthWriteEnable = VK_TRUE;
		config_info.depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
		config_info.depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
		config_info.depth_stencil_info.minDepthBounds = 0.0f;	// optional
		config_info.depth_stencil_info.maxDepthBounds = 1.0f;	// optional
		config_info.depth_stencil_info.stencilTestEnable = VK_FALSE;
		config_info.depth_stencil_info.front = {};	// optional
		config_info.depth_stencil_info.back = {};	// optional

		config_info.dynamic_state_enables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		config_info.dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		config_info.dynamic_state_info.pDynamicStates = config_info.dynamic_state_enables.data();
		config_info.dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(config_info.dynamic_state_enables.size());
		config_info.dynamic_state_info.flags = 0;
	}

	CvlPipeline::CvlPipeline
	(
		CvlDevice& device,
		const PipelineConfigInfo& config_info,
		const std::string& v_shader_fp,
		const std::string& f_shader_fp
	) : _cvl_device(device)
	{
		CreateGraphicsPipeline(v_shader_fp, f_shader_fp, config_info);
	}

	CvlPipeline::~CvlPipeline()
	{
		vkDestroyShaderModule(_cvl_device.device(), _v_shader_module, nullptr);
		vkDestroyShaderModule(_cvl_device.device(), _f_shader_module, nullptr);
		vkDestroyPipeline(_cvl_device.device(), _graphics_pipeline, nullptr);
	}

	void CvlPipeline::Bind(VkCommandBuffer command_buffer)
	{
		// BIND_POINT: _GRAPHICS, _COMPUTE, _RAY_TRACING
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphics_pipeline);
	}

	void CvlPipeline::CreateGraphicsPipeline(const std::string& v_shader_fp, const std::string& f_shader_fp, const PipelineConfigInfo& config_info)
	{
		assert(config_info.pipeline_layout != VK_NULL_HANDLE);
		assert(config_info.render_pass != VK_NULL_HANDLE);
		auto v_shader_code = ReadFile(CompileShader(std::filesystem::current_path().string() + '\\' + v_shader_fp));
		auto f_shader_code = ReadFile(CompileShader(std::filesystem::current_path().string() + '\\' + f_shader_fp));

		std::cout << "Vertex shader code size: " << v_shader_code.size() << '\n';
		std::cout << "Fragment shader code size: " << f_shader_code.size() << '\n';

		CreateShaderModule(v_shader_code, &_v_shader_module);
		CreateShaderModule(f_shader_code, &_f_shader_module);

		VkPipelineShaderStageCreateInfo shader_stages[2];
		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = _v_shader_module;
		shader_stages[0].pName = "main";
		shader_stages[0].flags = 0;
		shader_stages[0].pNext = nullptr;
		shader_stages[0].pSpecializationInfo = nullptr;
		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = _f_shader_module;
		shader_stages[1].pName = "main";
		shader_stages[1].flags = 0;
		shader_stages[1].pNext = nullptr;
		shader_stages[1].pSpecializationInfo = nullptr;

		auto attribute_descriptions = CvlModel::Vertex::GetAttributeDescriptions();
		auto binding_descriptions = CvlModel::Vertex::GetBindingDescriptions();
		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
		vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size());
		vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();
		vertex_input_info.pVertexBindingDescriptions = binding_descriptions.data();;

		VkGraphicsPipelineCreateInfo pipeline_info{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stages;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &config_info.input_assembly_info;
		pipeline_info.pViewportState = &config_info.viewport_info;
		pipeline_info.pRasterizationState = &config_info.rasterization_info;
		pipeline_info.pMultisampleState = &config_info.multisample_info;
		pipeline_info.pColorBlendState = &config_info.color_blend_info;
		pipeline_info.pDepthStencilState = &config_info.depth_stencil_info;
		pipeline_info.pDynamicState = &config_info.dynamic_state_info;

		pipeline_info.layout = config_info.pipeline_layout;
		pipeline_info.renderPass = config_info.render_pass;
		pipeline_info.subpass = config_info.subpass;

		pipeline_info.basePipelineIndex = -1;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(_cvl_device.device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_graphics_pipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlPipeline] Failed to create graphics pipeline!");
		}
	}

	void CvlPipeline::CreateShaderModule(const std::vector<char>& code, VkShaderModule* shader_module)
	{
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		// default allocator of vector ensures that the data satisfies the worst case alignment requirement
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		if (vkCreateShaderModule(_cvl_device.device(), &create_info, nullptr, shader_module) != VK_SUCCESS)
		{
			throw std::runtime_error("[CvlPipeline] Failed to create shader module!");
		}
	}

}