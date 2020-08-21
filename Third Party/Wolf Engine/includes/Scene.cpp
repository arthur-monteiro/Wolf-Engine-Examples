#include "Scene.h"
#include "InputVertexTemplate.h"
#include "Debug.h"

Wolf::Scene::Scene(SceneCreateInfo createInfo, VkDevice device, VkPhysicalDevice physicalDevice, std::vector<Image*> swapChainImages, VkCommandPool graphicsCommandPool, VkCommandPool computeCommandPool)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_swapChainImages = std::move(swapChainImages);
	m_swapChainCommandType = createInfo.swapChainCommandType;

	m_graphicsCommandPool = graphicsCommandPool;
	m_computeCommandPool = computeCommandPool;
}

Wolf::Scene::Scene(SceneCreateInfo createInfo, VkDevice device, VkPhysicalDevice physicalDevice,
	std::vector<Image*> ovrSwapChainImages, std::vector<Image*> windowSwapChainImages,
	VkCommandPool graphicsCommandPool, VkCommandPool computeCommandPool)
{
	m_useOVR = true;

	m_device = device;
	m_physicalDevice = physicalDevice;
	m_swapChainImages = std::move(ovrSwapChainImages);
	m_swapChainCommandType = createInfo.swapChainCommandType;

	m_graphicsCommandPool = graphicsCommandPool;
	m_computeCommandPool = computeCommandPool;
	m_windowSwapChainImages = std::move(windowSwapChainImages);
}

int Wolf::Scene::addRenderPass(Wolf::Scene::RenderPassCreateInfo createInfo)
{
	if(createInfo.outputIsSwapChain)
	{
		createInfo.outputs.resize(2);
		
		createInfo.outputs[0].clearValue = { 1.0f };
		createInfo.outputs[0].attachment = Attachment({ m_swapChainImages[0]->getExtent().width, m_swapChainImages[0]->getExtent().height }, findDepthFormat(m_physicalDevice), 
			VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		createInfo.outputs[1].clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
		createInfo.outputs[1].attachment = Attachment({ m_swapChainImages[0]->getExtent().width, m_swapChainImages[0]->getExtent().height }, m_swapChainImages[0]->getFormat(), 
			VK_SAMPLE_COUNT_1_BIT, m_useOVR ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	}
	else
	{
		if (createInfo.outputs.empty() && createInfo.extent.width == 0)
		{
			Debug::sendError("RenderPass creation must include output");
			return -1;
		}
		else if (createInfo.extent.width == 0)
			createInfo.extent = createInfo.outputs[0].attachment.extent;

		bool depthAttachmentPresent = false;
		for (RenderPassOutput& output : createInfo.outputs)
		{
			if (output.attachment.usageType & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
				depthAttachmentPresent = true;

			if (output.attachment.extent.width == 0 || output.attachment.extent.height == 0)
				output.attachment.extent = { m_swapChainImages[0]->getExtent().width, m_swapChainImages[0]->getExtent().height };

		}
	}
	
	m_sceneRenderPasses.emplace_back(createInfo.commandBufferID, createInfo.outputs, createInfo.outputIsSwapChain);

	std::vector<Attachment> attachments(0);
	for (RenderPassOutput& output : m_sceneRenderPasses[m_sceneRenderPasses.size() - 1].outputs)
		attachments.push_back(output.attachment);

	// Render pass creation
	if (m_sceneRenderPasses[m_sceneRenderPasses.size() - 1].outputIsSwapChain)
		m_sceneRenderPasses[m_sceneRenderPasses.size() - 1].renderPass = std::make_unique<RenderPass>(m_device, m_physicalDevice, m_graphicsCommandPool, attachments, m_swapChainImages);
	else
		m_sceneRenderPasses[m_sceneRenderPasses.size() - 1].renderPass = std::make_unique<RenderPass>(m_device, m_physicalDevice, m_graphicsCommandPool, attachments, 
			std::vector<VkExtent2D>(1, createInfo.extent));

	return static_cast<int>(m_sceneRenderPasses.size() - 1);
}

int Wolf::Scene::addComputePass(ComputePassCreateInfo createInfo)
{
	m_sceneComputePasses.emplace_back(createInfo.commandBufferID, createInfo.outputIsSwapChain);

	if(!createInfo.outputIsSwapChain)
	{
		m_sceneComputePasses.back().computePasses.resize(1);
		m_sceneComputePasses.back().computePasses[0] = std::make_unique<ComputePass>(m_device, m_physicalDevice, m_computeCommandPool, createInfo.computeShaderPath, 
			createInfo.descriptorSetCreateInfo);

		updateDescriptorPool(createInfo.descriptorSetCreateInfo);
	}
	else
	{
		m_descriptorPool.addStorageImage(static_cast<uint32_t>(m_swapChainImages.size()));
		
		m_sceneComputePasses.back().computePasses.resize(m_swapChainImages.size());
		for(size_t i(0); i < m_swapChainImages.size(); ++i)
		{
			std::vector<std::pair<std::vector<DescriptorSetCreateInfo::ImageData>, DescriptorLayout>> images;
			for (size_t j(0); j < createInfo.descriptorSetCreateInfo.descriptorImages.size(); ++j)
				images.push_back(createInfo.descriptorSetCreateInfo.descriptorImages[j]);

			DescriptorLayout swapChainImageLayout;
			swapChainImageLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			swapChainImageLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
			swapChainImageLayout.count = 1;
			swapChainImageLayout.binding = createInfo.outputBinding;

			DescriptorSetCreateInfo::ImageData swapChainImageData{};
			swapChainImageData.image = m_swapChainImages[i];
			
			images.push_back({ { swapChainImageData }, swapChainImageLayout });

			auto tempDescriptorSetCreateInfo = createInfo.descriptorSetCreateInfo;
			tempDescriptorSetCreateInfo.descriptorImages = images;

			m_sceneComputePasses.back().computePasses[i] = std::make_unique<ComputePass>(m_device, m_physicalDevice, m_computeCommandPool, createInfo.computeShaderPath,
				tempDescriptorSetCreateInfo);
			
			updateDescriptorPool(createInfo.descriptorSetCreateInfo);
		}

		createInfo.extent = { m_swapChainImages[0]->getExtent().width, m_swapChainImages[0]->getExtent().height };
	}
	
	m_sceneComputePasses.back().extent = createInfo.extent;
	m_sceneComputePasses.back().dispatchGroups = createInfo.dispatchGroups;

	m_sceneComputePasses.back().beforeRecord = createInfo.beforeRecord;
	m_sceneComputePasses.back().dataForBeforeRecordCallback = createInfo.dataForBeforeRecordCallback;
	m_sceneComputePasses.back().afterRecord = createInfo.afterRecord;
	m_sceneComputePasses.back().dataForAfterRecordCallback = createInfo.dataForAfterRecordCallback;

	return static_cast<int>(m_sceneComputePasses.size() - 1);
}

int Wolf::Scene::addCommandBuffer(CommandBufferCreateInfo createInfo)
{
	m_sceneCommandBuffers.emplace_back(createInfo.commandType);
	
	if (createInfo.commandType == CommandType::GRAPHICS)
		m_sceneCommandBuffers.back().commandBuffer = std::make_unique<CommandBuffer>(m_device, m_graphicsCommandPool);
	else if (createInfo.commandType == CommandType::COMPUTE)
		m_sceneCommandBuffers.back().commandBuffer = std::make_unique<CommandBuffer>(m_device, m_computeCommandPool);
	else
		Debug::sendError("Invalid command type");
	
	m_sceneCommandBuffers.back().semaphore = std::make_unique<Semaphore>();
	m_sceneCommandBuffers.back().semaphore->initialize(m_device);
	m_sceneCommandBuffers.back().semaphore->setPipelineStage(createInfo.finalPipelineStage);

	return static_cast<int>(m_sceneCommandBuffers.size() - 1);
}

int Wolf::Scene::addRenderer(RendererCreateInfo createInfo)
{
#ifdef _DEBUG
	if(createInfo.renderPassID < 0 || createInfo.renderPassID > m_sceneRenderPasses.size() - 1)
	{
		Debug::sendError("Invalid render pass ID. ID sent = " + std::to_string(createInfo.renderPassID) + ", last render pass ID = " + std::to_string(m_sceneRenderPasses.size() - 1));
		return -1;
	}
#endif

	// Set input attribut and binding descriptions from template
	switch (createInfo.inputVerticesTemplate)
	{
	case InputVertexTemplate::POSITION_2D :
		createInfo.pipelineCreateInfo.vertexInputAttributeDescriptions = Vertex2D::getAttributeDescriptions(0);
		createInfo.pipelineCreateInfo.vertexInputBindingDescriptions = { Vertex2D::getBindingDescription(0) };
		break;
	case InputVertexTemplate::POSITION_TEXTURECOORD_2D:
		createInfo.pipelineCreateInfo.vertexInputAttributeDescriptions = Vertex2DTextured::getAttributeDescriptions(0);
		createInfo.pipelineCreateInfo.vertexInputBindingDescriptions = { Vertex2DTextured::getBindingDescription(0) };
		break;
	case InputVertexTemplate::POSITION_TEXTURECOORD_ID_2D:
		createInfo.pipelineCreateInfo.vertexInputAttributeDescriptions = Vertex2DTexturedWithMaterial::getAttributeDescriptions(0);
		createInfo.pipelineCreateInfo.vertexInputBindingDescriptions = { Vertex2DTexturedWithMaterial::getBindingDescription(0) };
		break;
	case InputVertexTemplate::FULL_3D_MATERIAL:
		createInfo.pipelineCreateInfo.vertexInputAttributeDescriptions = Vertex3D::getAttributeDescriptions(0);
		createInfo.pipelineCreateInfo.vertexInputBindingDescriptions = { Vertex3D::getBindingDescription(0) };
		break;		
	case InputVertexTemplate::NO:
		break;
	default:
		Debug::sendError("Unknown inputVerticesTemplate while creating renderer");
		break;
	}

	switch (createInfo.instanceTemplate)
	{
	case InstanceTemplate::SINGLE_ID:
		std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions = InstanceSingleID::getAttributeDescriptions(1, 2);
		std::vector<VkVertexInputBindingDescription> inputBindingDescriptions = { InstanceSingleID::getBindingDescription(1) };

		for (VkVertexInputAttributeDescription& inputAttributeDescription : inputAttributeDescriptions)
			createInfo.pipelineCreateInfo.vertexInputAttributeDescriptions.push_back(inputAttributeDescription);
		for (VkVertexInputBindingDescription& inputBindingDescription : inputBindingDescriptions)
			createInfo.pipelineCreateInfo.vertexInputBindingDescriptions.push_back(inputBindingDescription);
		break;
	}

	if (createInfo.pipelineCreateInfo.extent.width == 0)
		createInfo.pipelineCreateInfo.extent = { m_swapChainImages[0]->getExtent().width, m_swapChainImages[0]->getExtent().height };

	createInfo.pipelineCreateInfo.renderPass = m_sceneRenderPasses[createInfo.renderPassID].renderPass->getRenderPass();

	auto* const r = new Renderer(m_device, createInfo);
	
	m_sceneRenderPasses[createInfo.renderPassID].renderers.push_back(std::unique_ptr<Renderer>(r));
	//m_sceneRenderPasses[createInfo.renderPassID].renderers.back()->setViewport(createInfo.pipelineCreateInfo.viewportScale, createInfo.pipelineCreateInfo.viewportOffset);

	return static_cast<int>(m_sceneRenderPasses[createInfo.renderPassID].renderers.size() - 1);
}

void Wolf::Scene::addMesh(Renderer::AddMeshInfo addMeshInfo)
{
	updateDescriptorPool(addMeshInfo.descriptorSetCreateInfo);

	m_sceneRenderPasses[addMeshInfo.renderPassID].renderers[addMeshInfo.rendererID]->addMesh(addMeshInfo);
}

void Wolf::Scene::addText(AddTextInfo addTextInfo)
{	
	// Build text
	const VkExtent2D outputExtent = m_sceneRenderPasses[addTextInfo.renderPassID].outputs[0].attachment.extent;
	addTextInfo.text->build(outputExtent, addTextInfo.font, addTextInfo.size);

	DescriptorSetGenerator descriptorSetGenerator;

	// Uniform Buffer Objects
	descriptorSetGenerator.addUniformBuffer(addTextInfo.text->getUBO(), VK_SHADER_STAGE_VERTEX_BIT, 0);

	// Images
	descriptorSetGenerator.addImages(addTextInfo.font->getImages(), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 2);

	// Samplers
	descriptorSetGenerator.addSampler(addTextInfo.font->getSampler(), VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	// Info to add
	DescriptorSetCreateInfo descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();
	for (auto& buffer : addTextInfo.descriptorSetCreateInfo.descriptorBuffers)
		descriptorSetCreateInfo.descriptorBuffers.push_back(buffer);
	
	for (auto& image : addTextInfo.descriptorSetCreateInfo.descriptorImages)
		descriptorSetCreateInfo.descriptorImages.push_back(image);

	Renderer::AddMeshInfo addMeshInfo{};
	addMeshInfo.descriptorSetCreateInfo = descriptorSetCreateInfo;
	addMeshInfo.vertexBuffer = addTextInfo.text->getVertexBuffer();
	
	m_sceneRenderPasses[addTextInfo.renderPassID].renderers[addTextInfo.rendererID]->addMesh(addMeshInfo);

	// Update descriptor pools needs
	updateDescriptorPool(descriptorSetCreateInfo);
}

void Wolf::Scene::record()
{
	m_descriptorPool.allocate(m_device);
	
	for(SceneRenderPass& sceneRenderPass : m_sceneRenderPasses)
	{
		// Renderers creation
		for (std::unique_ptr<Renderer>& renderer : sceneRenderPass.renderers)
			renderer->create(m_descriptorPool.getDescriptorPool());
	}

	for (SceneComputePass& sceneComputePass : m_sceneComputePasses)
	{
		for(size_t i(0); i < sceneComputePass.computePasses.size(); ++i)
			sceneComputePass.computePasses[i]->create(m_descriptorPool.getDescriptorPool());
	}
	
	// As a scene is designed to be renderer on a screen, we need to create a command buffer for each swapchain image
	m_swapChainCommandBuffers.resize(m_swapChainImages.size());
	for (size_t i(0); i < m_swapChainImages.size(); ++i)
	{
		if(m_swapChainCommandType == CommandType::GRAPHICS)
			m_swapChainCommandBuffers[i] = std::make_unique<CommandBuffer>(m_device, m_graphicsCommandPool);
		else 
			m_swapChainCommandBuffers[i] = std::make_unique<CommandBuffer>(m_device, m_computeCommandPool);
		
		m_swapChainCommandBuffers[i]->beginCommandBuffer();

		if(m_swapChainCommandType == CommandType::GRAPHICS)
		{
			for (size_t j(0); j < m_sceneRenderPasses.size(); ++j)
			{
				if (m_sceneRenderPasses[j].commandBufferID == -1)
				{
					std::vector<VkClearValue> clearValues(0);
					for (RenderPassOutput& output : m_sceneRenderPasses[j].outputs)
						clearValues.push_back(output.clearValue);

					if (m_sceneRenderPasses[j].outputIsSwapChain)
						m_sceneRenderPasses[j].renderPass->beginRenderPass(i, clearValues, m_swapChainCommandBuffers[i]->getCommandBuffer());
					else
						m_sceneRenderPasses[j].renderPass->beginRenderPass(0, clearValues, m_swapChainCommandBuffers[i]->getCommandBuffer());

					for (std::unique_ptr<Renderer>& renderer : m_sceneRenderPasses[j].renderers)
					{
						vkCmdBindPipeline(m_swapChainCommandBuffers[i]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->getPipeline());
						const VkDeviceSize offsets[1] = { 0 };

						/*VkViewport viewport;
						viewport.x = 0;
						viewport.y = 0;
						viewport.height = 100;
						viewport.width = 100;
						viewport.minDepth = 0.0f;
						viewport.maxDepth = 1.0f;
						vkCmdSetViewport(m_swapChainCommandBuffers[i]->getCommandBuffer(), 0, 1, &viewport);*/

						std::vector<std::tuple<VertexBuffer, InstanceBuffer, VkDescriptorSet>> meshesToRender = renderer->getMeshes();
						for (std::tuple<VertexBuffer, InstanceBuffer, VkDescriptorSet>& mesh : meshesToRender)
						{
							bool isInstancied = std::get<1>(mesh).nInstances > 0 && std::get<1>(mesh).instanceBuffer;

							vkCmdBindVertexBuffers(m_swapChainCommandBuffers[i]->getCommandBuffer(), 0, 1, &std::get<0>(mesh).vertexBuffer, offsets);
							vkCmdBindIndexBuffer(m_swapChainCommandBuffers[i]->getCommandBuffer(), std::get<0>(mesh).indexBuffer, 0, VK_INDEX_TYPE_UINT32);

							if (isInstancied)
								vkCmdBindVertexBuffers(m_swapChainCommandBuffers[i]->getCommandBuffer(), 1, 1, &std::get<1>(mesh).instanceBuffer, offsets);

							if (std::get<2>(mesh) != VK_NULL_HANDLE) // render can be done without descriptor set
								vkCmdBindDescriptorSets(m_swapChainCommandBuffers[i]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
									renderer->getPipelineLayout(), 0, 1, &std::get<2>(mesh), 0, nullptr);

							if (!isInstancied)
								vkCmdDrawIndexed(m_swapChainCommandBuffers[i]->getCommandBuffer(), std::get<0>(mesh).nbIndices, 1, 0, 0, 0);
							else
								vkCmdDrawIndexed(m_swapChainCommandBuffers[i]->getCommandBuffer(), std::get<0>(mesh).nbIndices, std::get<1>(meshesToRender[j]).nInstances, 0, 0, 0);
						}
					}

					m_sceneRenderPasses[j].renderPass->endRenderPass(m_swapChainCommandBuffers[i]->getCommandBuffer());

					// Copy result to mirror
					if (m_useOVR)
					{
						Image::transitionImageLayoutUsingCommandBuffer(m_swapChainCommandBuffers[i]->getCommandBuffer(), m_windowSwapChainImages[i]->getImage(), VK_FORMAT_R8G8B8A8_UNORM /* just no depth */, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0);

						VkImageBlit region = {};
						region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						region.srcSubresource.mipLevel = 0;
						region.srcSubresource.baseArrayLayer = 0;
						region.srcSubresource.layerCount = 1;
						region.srcOffsets[0] = { 0, 0, 0 };
						region.srcOffsets[1] = { static_cast<int32_t>(m_swapChainImages[0]->getExtent().width), static_cast<int32_t>(m_swapChainImages[0]->getExtent().height), 1 };
						region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						region.dstSubresource.mipLevel = 0;
						region.dstSubresource.baseArrayLayer = 0;
						region.dstSubresource.layerCount = 1;
						region.dstOffsets[0] = { 0, 0, 0 };
						region.dstOffsets[1] = { static_cast<int32_t>(m_windowSwapChainImages[i]->getExtent().width),  static_cast<int32_t>(m_windowSwapChainImages[i]->getExtent().height), 1 };
						vkCmdBlitImage(m_swapChainCommandBuffers[i]->getCommandBuffer(), m_swapChainImages[i]->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							m_windowSwapChainImages[i]->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);

						Image::transitionImageLayoutUsingCommandBuffer(m_swapChainCommandBuffers[i]->getCommandBuffer(), m_windowSwapChainImages[i]->getImage(), VK_FORMAT_R8G8B8A8_UNORM /* just no depth */, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
							1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
					}
				}
			}
		}
		else
		{
			for (size_t j(0); j < m_sceneComputePasses.size(); ++j)
			{
				if (m_sceneComputePasses[j].commandBufferID == -1)
				{
					Image::transitionImageLayoutUsingCommandBuffer(m_swapChainCommandBuffers[i]->getCommandBuffer(), m_swapChainImages[i]->getImage(), m_swapChainImages[i]->getFormat(),
						VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_GENERAL,
						1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0);

					m_sceneComputePasses[j].computePasses[i]->record(m_swapChainCommandBuffers[i]->getCommandBuffer(), 
						{ m_swapChainImages[i]->getExtent().width, m_swapChainImages[i]->getExtent().height }, m_sceneComputePasses[j].dispatchGroups);

					Image::transitionImageLayoutUsingCommandBuffer(m_swapChainCommandBuffers[i]->getCommandBuffer(), m_swapChainImages[i]->getImage(), m_swapChainImages[i]->getFormat(),
						VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
						1, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
				}
			}
		}
		
		m_swapChainCommandBuffers[i]->endCommandBuffer();
	}

	m_swapChainCompleteSemaphore = std::make_unique<Semaphore>();
	m_swapChainCompleteSemaphore->initialize(m_device);
	
	// Other command buffers
	for(size_t i(0); i < m_sceneCommandBuffers.size(); ++i)
	{
		m_sceneCommandBuffers[i].commandBuffer->beginCommandBuffer();

		for (auto& sceneRenderPass : m_sceneRenderPasses)
		{
			if (sceneRenderPass.commandBufferID == static_cast<int>(i))
			{
				recordRenderPass(sceneRenderPass);
			}
		}

		for(auto& sceneComputePass : m_sceneComputePasses)
		{
			if(sceneComputePass.commandBufferID == static_cast<int>(i))
			{
				if(sceneComputePass.beforeRecord)
					sceneComputePass.beforeRecord(sceneComputePass.dataForBeforeRecordCallback, m_sceneCommandBuffers[sceneComputePass.commandBufferID].commandBuffer->getCommandBuffer());
				
				for(size_t j(0); j < sceneComputePass.computePasses.size(); ++j)
					sceneComputePass.computePasses[j]->record(m_sceneCommandBuffers[sceneComputePass.commandBufferID].commandBuffer->getCommandBuffer(), sceneComputePass.extent, 
						sceneComputePass.dispatchGroups);

				if (sceneComputePass.afterRecord)
					sceneComputePass.afterRecord(sceneComputePass.dataForAfterRecordCallback, m_sceneCommandBuffers[sceneComputePass.commandBufferID].commandBuffer->getCommandBuffer());
			}
		}
		
		m_sceneCommandBuffers[i].commandBuffer->endCommandBuffer();
	}
}

inline void Wolf::Scene::recordRenderPass(SceneRenderPass& sceneRenderPasse)
{
	std::vector<VkClearValue> clearValues(0);
	for (RenderPassOutput& output : sceneRenderPasse.outputs)
		clearValues.push_back(output.clearValue);

	sceneRenderPasse.renderPass->beginRenderPass(0, clearValues, m_sceneCommandBuffers[sceneRenderPasse.commandBufferID].commandBuffer->getCommandBuffer());

	for (std::unique_ptr<Renderer>& renderer : sceneRenderPasse.renderers)
	{
		vkCmdBindPipeline(m_sceneCommandBuffers[sceneRenderPasse.commandBufferID].commandBuffer->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->getPipeline());
		const VkDeviceSize offsets[1] = { 0 };

		std::vector<std::tuple<VertexBuffer, InstanceBuffer, VkDescriptorSet>> meshesToRender = renderer->getMeshes();
		for (std::tuple<VertexBuffer, InstanceBuffer, VkDescriptorSet>& mesh : meshesToRender)
		{
			bool isInstancied = std::get<1>(mesh).nInstances > 0 && std::get<1>(mesh).instanceBuffer;

			vkCmdBindVertexBuffers(m_sceneCommandBuffers[sceneRenderPasse.commandBufferID].commandBuffer->getCommandBuffer(), 0, 1, &std::get<0>(mesh).vertexBuffer, offsets);
			vkCmdBindIndexBuffer(m_sceneCommandBuffers[sceneRenderPasse.commandBufferID].commandBuffer->getCommandBuffer(), std::get<0>(mesh).indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			if (isInstancied)
				vkCmdBindVertexBuffers(m_sceneCommandBuffers[sceneRenderPasse.commandBufferID].commandBuffer->getCommandBuffer(), 1, 1, &std::get<1>(mesh).instanceBuffer, offsets);

			if (std::get<2>(mesh) != VK_NULL_HANDLE) // render can be done without descriptor set
				vkCmdBindDescriptorSets(m_sceneCommandBuffers[sceneRenderPasse.commandBufferID].commandBuffer->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
					renderer->getPipelineLayout(), 0, 1, &std::get<2>(mesh), 0, nullptr);

			if (!isInstancied)
				vkCmdDrawIndexed(m_sceneCommandBuffers[sceneRenderPasse.commandBufferID].commandBuffer->getCommandBuffer(), std::get<0>(mesh).nbIndices, 1, 0, 0, 0);
			else
				vkCmdDrawIndexed(m_sceneCommandBuffers[sceneRenderPasse.commandBufferID].commandBuffer->getCommandBuffer(), std::get<0>(mesh).nbIndices, std::get<1>(mesh).nInstances, 0, 0, 0);
		}
	}

	sceneRenderPasse.renderPass->endRenderPass(m_sceneCommandBuffers[sceneRenderPasse.commandBufferID].commandBuffer->getCommandBuffer());
}

void Wolf::Scene::frame(Queue graphicsQueue, Queue computeQueue, uint32_t swapChainImageIndex, Semaphore* imageAvailableSemaphore, std::vector<int> commandBufferIDs,
                        const std::vector<std::pair<int, int>>& commandBufferSynchronization)
{
	for(auto& commandBufferID : commandBufferIDs)
	{
		if (commandBufferID < 0)
			continue;

		std::vector<Semaphore*> waitSemaphores;
		for (auto& commandBufferWaiting : commandBufferSynchronization)
		{
			if (commandBufferWaiting.second == commandBufferID)
			{
				waitSemaphores.push_back(m_sceneCommandBuffers[commandBufferWaiting.first].semaphore.get());
			}
		}

		if (m_sceneCommandBuffers[commandBufferID].type == CommandType::GRAPHICS)
			m_sceneCommandBuffers[commandBufferID].commandBuffer->submit(m_device, graphicsQueue, waitSemaphores, { m_sceneCommandBuffers[commandBufferID].semaphore->getSemaphore() });
		else if (m_sceneCommandBuffers[commandBufferID].type == CommandType::COMPUTE)
			m_sceneCommandBuffers[commandBufferID].commandBuffer->submit(m_device, computeQueue, waitSemaphores, { m_sceneCommandBuffers[commandBufferID].semaphore->getSemaphore() });
		else
			Debug::sendError("Invalid queue type at sumbit");
	}

	std::vector<Semaphore*> waitSemaphoreSwapChain;
	std::vector<VkSemaphore> signalSemaphoreSwapChain;
	if (imageAvailableSemaphore)
	{
		waitSemaphoreSwapChain.push_back(imageAvailableSemaphore);
		signalSemaphoreSwapChain.push_back(m_swapChainCompleteSemaphore->getSemaphore());	
	}

	for(auto& commandBufferWaiting : commandBufferSynchronization)
	{
		if(commandBufferWaiting.second == -1)
		{
			if (commandBufferWaiting.first == -1)
				Debug::sendError("No command buffer can't wait from swapchain command buffer");
			else if (commandBufferWaiting.first < 0)
				Debug::sendError("Invalid command buffer ID");
			waitSemaphoreSwapChain.push_back(m_sceneCommandBuffers[commandBufferWaiting.first].semaphore.get());
		}
	}

	if(m_swapChainCommandType == CommandType::GRAPHICS)
		m_swapChainCommandBuffers[swapChainImageIndex]->submit(m_device, graphicsQueue, waitSemaphoreSwapChain, signalSemaphoreSwapChain);
	else
		m_swapChainCommandBuffers[swapChainImageIndex]->submit(m_device, computeQueue, waitSemaphoreSwapChain, signalSemaphoreSwapChain);
}

inline void Wolf::Scene::updateDescriptorPool(Wolf::DescriptorSetCreateInfo& descriptorSetCreateInfo)
{
	for (auto& descriptorBuffer : descriptorSetCreateInfo.descriptorBuffers)
	{
		switch (descriptorBuffer.second.descriptorType)
		{
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			m_descriptorPool.addUniformBuffer(descriptorBuffer.second.count);
			break;

		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			m_descriptorPool.addStorageBuffer(descriptorBuffer.second.count);
			break;

		default: Debug::sendWarning("Unsupported descriptor buffer type");
		}
	}

	for (auto& descriptorImage : descriptorSetCreateInfo.descriptorImages)
	{
		switch (descriptorImage.second.descriptorType)
		{
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			m_descriptorPool.addStorageImage(descriptorImage.second.count);
			break;

		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			m_descriptorPool.addCombinedImageSampler(descriptorImage.second.count);
			break;

		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			m_descriptorPool.addSampledImage(descriptorImage.second.count);
			break;

		case VK_DESCRIPTOR_TYPE_SAMPLER:
			m_descriptorPool.addSampler(descriptorImage.second.count);
			break;

		default: Debug::sendWarning("Unsupported descriptor image type");
		}
	}
}