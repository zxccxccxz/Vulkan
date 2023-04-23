#pragma once

#include "cvl_device.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>

namespace cvl
{
	class CvlModel
	{
	public:
		struct Vertex
		{
			glm::vec2 pos;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
		};

		CvlModel(CvlDevice& device, const std::vector<Vertex>& vertices);
		~CvlModel();

		CvlModel(const CvlModel&) = delete;
		CvlModel& operator=(const CvlModel&) = delete;

		void Bind(VkCommandBuffer command_buffer);
		void Draw(VkCommandBuffer command_buffer);

	private:
		void CreateVertexBuffers(const std::vector<Vertex>& vertices);

		CvlDevice& _cvl_device;
		VkBuffer _vertex_buffer;
		VkDeviceMemory _vertex_buffer_memory;
		uint32_t _vertex_count;
	};
}