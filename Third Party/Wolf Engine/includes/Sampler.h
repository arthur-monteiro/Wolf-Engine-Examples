#pragma once

#include "VulkanElement.h"

namespace Wolf
{	
	class Sampler : public VulkanElement
	{
	public:
		Sampler(VkDevice device, VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter, float maxAnisotropy = 16.0f);
		~Sampler();

		VkSampler getSampler() { return m_textureSampler; }

	private:
		VkSampler m_textureSampler = VK_NULL_HANDLE;
	};


}
