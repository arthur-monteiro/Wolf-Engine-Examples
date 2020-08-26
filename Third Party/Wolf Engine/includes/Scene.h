#pragma once

#include <cstdint>
#include <utility>

#include "VulkanElement.h"
#include "Image.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "RenderPass.h"
#include "Renderer.h"
#include "InputVertexTemplate.h"
#include "Model.h"
#include "DescriptorPool.h"
#include "Text.h"
#include "InstanceTemplate.h"
#include "ComputePass.h"
#include "RayTracingPass.h"

namespace Wolf
{
	
	class Scene : public VulkanElement
	{
	public:
		enum class CommandType { GRAPHICS, COMPUTE, TRANSFER, RAY_TRACING };
		struct	SceneCreateInfo
		{
			CommandType swapChainCommandType = CommandType::GRAPHICS;
		};
		
		Scene(SceneCreateInfo createInfo, VkDevice device, VkPhysicalDevice physicalDevice, std::vector<Image*> swapChainImages, VkCommandPool graphicsCommandPool, VkCommandPool computeCommandPool);
		Scene(SceneCreateInfo createInfo, VkDevice device, VkPhysicalDevice physicalDevice, std::vector<Image*> ovrSwapChainImages, std::vector<Image*> windowSwapChainImages, VkCommandPool graphicsCommandPool, VkCommandPool computeCommandPool);

		struct RenderPassOutput
		{
			VkClearValue clearValue;
			Attachment attachment;
		};

		struct RenderPassCreateInfo
		{
			int commandBufferID;
			bool outputIsSwapChain = false;

			// Output
			std::vector<RenderPassOutput> outputs;
			VkExtent2D extent = { 0, 0 };
		};
		int addRenderPass(RenderPassCreateInfo createInfo, int forceID = -1);

		struct ComputePassCreateInfo
		{
			int commandBufferID;
			bool outputIsSwapChain = false;
			uint32_t outputBinding = 0;
			VkExtent2D extent;
			VkExtent3D dispatchGroups;
			
			std::string computeShaderPath;

			DescriptorSetCreateInfo descriptorSetCreateInfo;

			std::function<void(void*, VkCommandBuffer)> beforeRecord = nullptr; void* dataForBeforeRecordCallback = nullptr;
			std::function<void(void*, VkCommandBuffer)> afterRecord = nullptr; void* dataForAfterRecordCallback = nullptr;
		};
		int addComputePass(ComputePassCreateInfo createInfo);

		struct RayTracingPassAddInfo
		{
			RayTracingPass::RayTracingPassCreateInfo rayTracingPassCreateInfo;

			int commandBufferID;
			bool outputIsSwapChain = false;
			uint32_t outputBinding = 0;
			VkExtent2D extent;

			std::function<void(void*, VkCommandBuffer)> beforeRecord = nullptr; void* dataForBeforeRecordCallback = nullptr;
			std::function<void(void*, VkCommandBuffer)> afterRecord = nullptr; void* dataForAfterRecordCallback = nullptr;
		};
		int addRayTracingPass(RayTracingPassAddInfo rayTracingPassAddInfo);

		struct CommandBufferCreateInfo
		{
			CommandType commandType = CommandType::COMPUTE;
			VkPipelineStageFlags finalPipelineStage;
		};
		int addCommandBuffer(CommandBufferCreateInfo createInfo);

		int addRenderer(RendererCreateInfo createInfo);

		void addMesh(Renderer::AddMeshInfo addMeshInfo);

		struct AddTextInfo
		{
			Text* text;
			Font* font;
			float size;
			int renderPassID;
			int rendererID;

			// Info to add
			DescriptorSetCreateInfo descriptorSetCreateInfo;
		};
		void addText(AddTextInfo addTextInfo);
		
		void record();
		
		void frame(Queue graphicsQueue, Queue computeQueue, uint32_t swapChainImageIndex, Semaphore* imageAvailableSemaphore, std::vector<int> commandBufferIDs,
		           const std::vector<std::pair<int, int>>&);

		void resize(std::vector<Image*> swapChainImages);

		VkSemaphore getSwapChainSemaphore() const { return m_swapChainCompleteSemaphore->getSemaphore(); }
		Image* getRenderPassOutput(int renderPassID, int textureID) { return m_sceneRenderPasses[renderPassID].renderPass->getImages(0)[textureID]; }		

	private:
		// Command Pools
		VkCommandPool m_graphicsCommandPool;
		VkCommandPool m_computeCommandPool;

		// Descriptor Pool
		DescriptorPool m_descriptorPool;
		
		// SwapChain
		std::vector<Image*> m_swapChainImages;
		std::vector<std::unique_ptr<CommandBuffer>> m_swapChainCommandBuffers;
		std::unique_ptr<Semaphore> m_swapChainCompleteSemaphore;
		CommandType m_swapChainCommandType = CommandType::GRAPHICS;

		// VR
		std::vector<Image*> m_windowSwapChainImages; // mirror images

		// CommandBuffer
		struct SceneCommandBuffer
		{
			std::unique_ptr<CommandBuffer> commandBuffer;
			std::unique_ptr<Semaphore> semaphore;
			CommandType type;

			SceneCommandBuffer(CommandType type)
			{
				this->type = type;
			}
		};
		std::vector<SceneCommandBuffer> m_sceneCommandBuffers;

		// RenderPasses
		struct SceneRenderPass
		{
			std::unique_ptr<RenderPass> renderPass;
			int commandBufferID;

			// Output
			std::vector<RenderPassOutput> outputs;
			bool outputIsSwapChain = false;

			std::vector<std::unique_ptr<Renderer>> renderers;

			SceneRenderPass(int commandBufferID, std::vector<RenderPassOutput> outputs, bool outputIsSwapChain)
			{
				this->outputs = std::move(outputs);
				this->outputIsSwapChain = outputIsSwapChain;
				this->commandBufferID = commandBufferID;
			}
		};
		std::vector<SceneRenderPass> m_sceneRenderPasses;

		struct SceneComputePass
		{
			std::vector<std::unique_ptr<ComputePass>> computePasses;
			int commandBufferID;
			
			bool outputIsSwapChain = false;
			uint32_t outputBinding = 0;
			VkExtent2D extent;
			VkExtent3D dispatchGroups;

			std::function<void(void*, VkCommandBuffer)> beforeRecord = nullptr; void* dataForBeforeRecordCallback = nullptr;
			std::function<void(void*, VkCommandBuffer)> afterRecord = nullptr; void* dataForAfterRecordCallback = nullptr;

			SceneComputePass(int commandBufferID, bool outputIsSwapChain)
			{
				this->outputIsSwapChain = outputIsSwapChain;
				this->commandBufferID = commandBufferID;
			}
		};
		std::vector<SceneComputePass> m_sceneComputePasses;

		struct SceneRayTracingPass
		{
			std::vector<std::unique_ptr<RayTracingPass>> rayTracingPasses;

			int commandBufferID;

			bool outputIsSwapChain = false;
			uint32_t outputBinding = 0;
			VkExtent2D extent;

			std::function<void(void*, VkCommandBuffer)> beforeRecord = nullptr; void* dataForBeforeRecordCallback = nullptr;
			std::function<void(void*, VkCommandBuffer)> afterRecord = nullptr; void* dataForAfterRecordCallback = nullptr;

			SceneRayTracingPass(int commandBufferID, bool outputIsSwapChain)
			{
				this->outputIsSwapChain = outputIsSwapChain;
				this->commandBufferID = commandBufferID;
			}
		};
		std::vector<SceneRayTracingPass> m_sceneRayTracingPasses;
		
		// VR
		bool m_useOVR = false;

	private:
		inline void updateDescriptorPool(DescriptorSetCreateInfo& descriptorSetCreateInfo);
		inline void recordRenderPass(SceneRenderPass& sceneRenderPasse);
	};
}
