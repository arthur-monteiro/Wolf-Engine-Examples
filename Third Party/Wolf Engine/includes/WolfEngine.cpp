#include "WolfEngine.h"

#include <utility>

Wolf::WolfInstance::WolfInstance(WolfInstanceCreateInfo createInfo)
{
	if (!createInfo.debugCallback)
		return;
	Debug::setCallback(createInfo.debugCallback);
	
	if (createInfo.windowHeight <= 0 || createInfo.windowHeight > MAX_HEIGHT)
		Debug::sendError("Window height is invalid or exceed max height. Height sent : " + std::to_string(createInfo.windowHeight) + ", maximum height = " + std::to_string(MAX_HEIGHT));
	if (createInfo.windowWidth <= 0 || createInfo.windowWidth > MAX_WIDTH)
		Debug::sendError("Window width is invalid or exceed max width. Width sent : " + std::to_string(createInfo.windowWidth) + ", maximum width = " + std::to_string(MAX_WIDTH));
	
	m_window = std::make_unique<Window>(createInfo.applicationName, createInfo.windowWidth, createInfo.windowHeight, this, windowResizeCallback);
	m_vulkan = std::make_unique<Vulkan>(m_window->getWindow(), createInfo.useOVR);
	m_swapChain = std::make_unique<SwapChain>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_vulkan->getSurface(), m_window->getWindow());

	m_graphicsCommandPool.initializeForGraphicsQueue(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_vulkan->getSurface());
	m_computeCommandPool.initializeForComputeQueue(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_vulkan->getSurface());

	m_useOVR = createInfo.useOVR;
	if (createInfo.useOVR)
	{
		m_ovr = std::make_unique<OVR>(m_vulkan->getDevice(), m_vulkan->getOVRSession(), m_vulkan->getGraphicsLuid());
	}
}

Wolf::Scene* Wolf::WolfInstance::createScene(Scene::SceneCreateInfo createInfo)
{
	if(m_useOVR)
		m_scenes.push_back(std::make_unique<Scene>(createInfo, m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_ovr->getImages(), m_swapChain->getImages(), m_graphicsCommandPool.getCommandPool(), m_computeCommandPool.getCommandPool()));
	else
		m_scenes.push_back(std::make_unique<Scene>(createInfo, m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_swapChain->getImages(), m_graphicsCommandPool.getCommandPool(), m_computeCommandPool.getCommandPool()));
	return m_scenes[m_scenes.size() - 1].get();
}

Wolf::UniformBufferObject* Wolf::WolfInstance::createUniformBufferObject()
{
	m_uniformBufferObjects.push_back(std::make_unique<UniformBufferObject>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice()));

	return m_uniformBufferObjects[m_uniformBufferObjects.size() - 1].get();
}

Wolf::Texture* Wolf::WolfInstance::createTexture()
{
	m_textures.push_back(std::make_unique<Texture>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue()));

	return m_textures[m_textures.size() - 1].get();
}

Wolf::Image* Wolf::WolfInstance::createImageFromFile(std::string filename)
{
	m_images.push_back(std::make_unique<Image>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue(), 
		filename));

	return m_images.back().get();
}

Wolf::Sampler* Wolf::WolfInstance::createSampler(VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter, float maxAnisotropy)
{
	m_samplers.push_back(std::make_unique<Sampler>(m_vulkan->getDevice(), addressMode, mipLevels, filter, maxAnisotropy));

	return m_samplers.back().get();
}

Wolf::Font* Wolf::WolfInstance::createFont()
{
	m_fonts.push_back(std::make_unique<Font>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue()));

	return m_fonts[m_fonts.size() - 1].get();
}

Wolf::Text* Wolf::WolfInstance::createText()
{
	m_texts.push_back(std::make_unique<Text>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue()));

	return m_texts[m_texts.size() - 1].get();
}

void Wolf::WolfInstance::updateOVR()
{
	m_ovr->update();
}

void Wolf::WolfInstance::frame(Scene* scene, std::vector<int> commandBufferIDs, std::vector<std::pair<int, int>> commandBufferSynchronisation)
{
	glfwPollEvents();

	if(m_useOVR)
	{
		const uint32_t swapChainImageIndex = m_ovr->getCurrentImage(m_vulkan->getDevice(), m_vulkan->getGraphicsQueue().queue);
		scene->frame(m_vulkan->getGraphicsQueue(), m_vulkan->getComputeQueue(), swapChainImageIndex, m_swapChain->getImageAvailableSemaphore(),
		             std::move(commandBufferIDs), std::move(commandBufferSynchronisation));
		m_ovr->present(swapChainImageIndex);

		const uint32_t windowSwapChainImageIndex = m_swapChain->getCurrentImage(m_vulkan->getDevice());
		m_swapChain->present(m_vulkan->getPresentQueue(), scene->getSwapChainSemaphore(), windowSwapChainImageIndex);
	}
	
	else
	{
		const uint32_t swapChainImageIndex = m_swapChain->getCurrentImage(m_vulkan->getDevice());
		scene->frame(m_vulkan->getGraphicsQueue(), m_vulkan->getComputeQueue(), swapChainImageIndex, m_swapChain->getImageAvailableSemaphore(), std::move(commandBufferIDs),
			std::move(commandBufferSynchronisation));
		m_swapChain->present(m_vulkan->getPresentQueue(), scene->getSwapChainSemaphore(), swapChainImageIndex);
	}
}

bool Wolf::WolfInstance::windowShouldClose()
{
	return glfwWindowShouldClose(m_window->getWindow());
}

void Wolf::WolfInstance::waitIdle()
{
	vkDeviceWaitIdle(m_vulkan->getDevice());
}
