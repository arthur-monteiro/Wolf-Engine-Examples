#pragma once

#include "Image.h"
#include "Sampler.h"
#include "UniformBufferObject.h"
#include "VulkanHelper.h"

namespace Wolf
{

	struct DescriptorLayout
	{
		VkDescriptorType descriptorType;
		VkShaderStageFlags accessibility{};
		uint32_t binding{};
		uint32_t count = 1;
	};
	
	struct DescriptorSetCreateInfo
	{
		struct BufferData
		{
			VkBuffer buffer;
			VkDeviceSize size;
		};
		std::vector<std::pair<std::vector<BufferData>, DescriptorLayout>> descriptorBuffers;

		struct ImageData
		{
			Image* image;
			Sampler* sampler;
		};
		std::vector<std::pair<std::vector<ImageData>, DescriptorLayout>> descriptorImages;
	};
	
	VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, DescriptorSetCreateInfo descriptorSetCreateInfo);

	class DescriptorSetGenerator
	{
	public:
		void addUniformBuffer(UniformBufferObject* ubo, VkShaderStageFlags accessibility, uint32_t binding);
		void addImages(std::vector<Image*> images, VkDescriptorType descriptorType, VkShaderStageFlags accessibility, uint32_t binding);
		void addCombinedImageSampler(Image* image, Sampler* sampler, VkShaderStageFlags accessibility, uint32_t binding);
		void addSampler(Sampler* sampler, VkShaderStageFlags accessibility, uint32_t binding);
		
		DescriptorSetCreateInfo getDescritorSetCreateInfo() { return m_descriptorSetCreateInfo; }
		
	private:
		DescriptorSetCreateInfo m_descriptorSetCreateInfo;
	};
}