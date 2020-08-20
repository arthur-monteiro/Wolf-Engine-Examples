#include "Renderer.h"

#include <utility>

#include "Debug.h"

Wolf::Renderer::Renderer(VkDevice device, RendererCreateInfo rendererCreateInfo)
{
	m_device = device;
	createDescriptorSetLayout(rendererCreateInfo.descriptorLayouts);

	rendererCreateInfo.pipelineCreateInfo.descriptorSetLayouts = { m_descriptorSetLayout };
	m_renderingPipelineCreate = rendererCreateInfo.pipelineCreateInfo;
}

Wolf::Renderer::~Renderer()
{
	for (size_t i(0); i < m_meshes.size(); ++i)
	{
		if(m_meshes[i].descriptorSet != VK_NULL_HANDLE) 
			vkFreeDescriptorSets(m_device, m_descriptorPool, 1, &m_meshes[i].descriptorSet);
	}
	vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

	m_meshes.clear();
}

int Wolf::Renderer::addMesh(AddMeshInfo addMeshInfo)
{
	m_meshes.emplace_back(addMeshInfo);

	return static_cast<int>(m_meshes.size() - 1);
}

void Wolf::Renderer::create(VkDescriptorPool descriptorPool)
{
	m_descriptorPool = descriptorPool;
	
	if (!m_pipeline)
		m_pipeline = std::make_unique<Pipeline>(m_device, m_renderingPipelineCreate);

	for(size_t i(0); i < m_meshes.size(); ++i)
	{
		if(m_meshes[i].descriptorSet == VK_NULL_HANDLE && m_meshes[i].needDescriptorSet())
		{
			m_meshes[i].descriptorSet = createDescriptorSet(m_device, m_descriptorSetLayout, descriptorPool, m_meshes[i].descriptorSetCreateInfo);
		}
	}
}

void Wolf::Renderer::setViewport(std::array<float, 2> viewportScale, std::array<float, 2> viewportOffset)
{
	m_renderingPipelineCreate.viewportScale = viewportScale;
	m_renderingPipelineCreate.viewportOffset = viewportOffset;
}

std::vector<std::tuple<Wolf::VertexBuffer, Wolf::InstanceBuffer, VkDescriptorSet>> Wolf::Renderer::getMeshes()
{
	std::vector<std::tuple<Wolf::VertexBuffer, Wolf::InstanceBuffer, VkDescriptorSet>> r(m_meshes.size());
	for(size_t i(0); i < m_meshes.size(); ++i)
	{
		r[i] = std::make_tuple(m_meshes[i].vertexBuffer, m_meshes[i].instanceBuffer, m_meshes[i].descriptorSet);
	}

	return r;
}

void Wolf::Renderer::createDescriptorSetLayout(const std::vector<DescriptorLayout>& descriptorLayouts)
{
	// Global bindings
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	
	for (auto descriptorLayout : descriptorLayouts)
	{
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
		descriptorSetLayoutBinding.binding = descriptorLayout.binding;
		descriptorSetLayoutBinding.descriptorType = descriptorLayout.descriptorType;
		descriptorSetLayoutBinding.descriptorCount = descriptorLayout.count;
		descriptorSetLayoutBinding.stageFlags = descriptorLayout.accessibility;
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(descriptorSetLayoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		Debug::sendError("Error : create descriptor set layout");
}