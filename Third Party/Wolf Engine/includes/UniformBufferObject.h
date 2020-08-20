#pragma once

#include "VulkanElement.h"

namespace Wolf
{
	class UniformBufferObject : public VulkanElement
	{
	public:
		UniformBufferObject(VkDevice device, VkPhysicalDevice physicalDevice);
		~UniformBufferObject();

		bool initializeData(void* data, VkDeviceSize size);

		void updateData(void* data);

		void cleanup();

		// Getters
	public:
		VkBuffer getUniformBuffer() const { return m_uniformBuffer; }
		VkDeviceSize getSize() const { return m_size; }

	private:
		VkBuffer m_uniformBuffer;
		VkDeviceMemory m_uniformBufferMemory;

		VkDeviceSize m_size = 0;
	};
}
