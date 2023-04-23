#include "cvl_model.h"

#include <cassert>
#include <cstring>

namespace cvl
{
	/* CvlModel class */
	CvlModel::CvlModel(CvlDevice& device, const std::vector<Vertex>& vertices)
		: _cvl_device(device)
	{
		CreateVertexBuffers(vertices);
	}

	CvlModel::~CvlModel()
	{
		// maxMemoryAllocationCount
		vkDestroyBuffer(_cvl_device.device(), _vertex_buffer, nullptr);
		vkFreeMemory(_cvl_device.device(), _vertex_buffer_memory, nullptr);
	}

	void CvlModel::Bind(VkCommandBuffer command_buffer)
	{
		VkBuffer buffers[] = { _vertex_buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);
	}

	void CvlModel::Draw(VkCommandBuffer command_buffer)
	{
		vkCmdDraw(command_buffer, _vertex_count, 1, 0, 0);
	}

	// private
	void CvlModel::CreateVertexBuffers(const std::vector<Vertex>& vertices)
	{
		_vertex_count = static_cast<uint32_t>(vertices.size());
		assert(_vertex_count >= 3 && "Vertex count must be at least 3");

		VkDeviceSize buffer_size = sizeof(vertices[0]) * _vertex_count;
		_cvl_device.CreateBuffer
		(
			buffer_size,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			// Host = CPU, Device = GPU
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			_vertex_buffer,
			_vertex_buffer_memory
		);
		// Data will point to the beggining of the mapped memory range (on gpu)
		void* data;
		vkMapMemory(_cvl_device.device(), _vertex_buffer_memory, 0, buffer_size, 0, &data);
		// Cpy into host map memory region, then the host memory will be automatically flushed to update the device memory
		memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
		// no need to call vkFlushMappedMemoryRanges beacuse VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is set
		vkUnmapMemory(_cvl_device.device(), _vertex_buffer_memory);
	}

	/* CvlModel::Vertex class */
	std::vector<VkVertexInputBindingDescription> CvlModel::Vertex::GetBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> binding_descriptions(1);
		binding_descriptions[0].binding = 0;
		binding_descriptions[0].stride = sizeof(Vertex);
		binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return binding_descriptions;
	}

	std::vector<VkVertexInputAttributeDescription> CvlModel::Vertex::GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attribute_descriptions(2);
		attribute_descriptions[0].location = 0;
		attribute_descriptions[0].binding = 0;
		attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions[0].offset = offsetof(Vertex, pos);

		attribute_descriptions[1].location = 1;
		attribute_descriptions[1].binding = 0;
		attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions[1].offset = offsetof(Vertex, color);
		return attribute_descriptions;
	}
	/* ~CvlModel::Vertex class */
	/* ~CvlModel class */
}