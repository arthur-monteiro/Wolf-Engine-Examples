#include <WolfEngine.h>

using namespace Wolf;

void debugCallback(Debug::Severity severity, std::string message)
{
	switch (severity)
	{
	case Debug::Severity::ERROR:
		std::cout << "Error : ";
		break;
	case Debug::Severity::WARNING:
		std::cout << "Warning : ";
		break;
	case Debug::Severity::INFO:
		std::cout << "Info : ";
		break;
	}

	std::cout << message << std::endl;
}

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define MAX_PARTICLE_COUNT 500
struct ParticleInfo
{
	glm::vec3 position;
	float bornTime;
	glm::vec3 velocity;
	float lifeTime;

	static VkVertexInputBindingDescription getBindingDescription(uint32_t binding)
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = binding;
		bindingDescription.stride = sizeof(ParticleInfo);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(uint32_t binding, uint32_t startLocation)
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

		attributeDescriptions[0].binding = binding;
		attributeDescriptions[0].location = startLocation;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		attributeDescriptions[1].binding = binding;
		attributeDescriptions[1].location = startLocation + 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = 0;

		return attributeDescriptions;
	}
};

struct FrameUniformBuffer
{
	float frameTime = 0.0f;
	float fullTime = 0.0f;
};

int main()
{
	WolfInstanceCreateInfo instanceCreateInfo;

	// Application
	instanceCreateInfo.applicationName = "This will be the title of the window";
	instanceCreateInfo.majorVersion = 1;
	instanceCreateInfo.minorVersion = 0;

	// Window
	instanceCreateInfo.windowHeight = WINDOW_HEIGHT;
	instanceCreateInfo.windowWidth = WINDOW_WIDTH;

	// Debug
	instanceCreateInfo.debugCallback = debugCallback;

	// Not a VR application
	instanceCreateInfo.useOVR = false;

	WolfInstance instance(instanceCreateInfo);

	// Scene creation
	Scene::SceneCreateInfo sceneCreateInfo;
	sceneCreateInfo.swapChainCommandType = Scene::CommandType::GRAPHICS;
	Scene* scene = instance.createScene(sceneCreateInfo); // the scene will be deleted with the wolf instance

	// Create particle buffer
	Buffer* particlesBuffer = instance.createBuffer(MAX_PARTICLE_COUNT * sizeof(ParticleInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Create frame uniform buffer
	FrameUniformBuffer frameUniformBufferData;
	UniformBuffer* frameUniformBuffer = instance.createUniformBufferObject(&frameUniformBufferData, sizeof(frameUniformBufferData));

	// Compute pass creation (creation, update and deletion of particles)
	Scene::ComputePassCreateInfo computePassCreateInfo{};

	Scene::CommandBufferCreateInfo addCommandBufferInfo{};
	addCommandBufferInfo.commandType = Scene::CommandType::COMPUTE;
	addCommandBufferInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	computePassCreateInfo.commandBufferID = scene->addCommandBuffer(addCommandBufferInfo);

	computePassCreateInfo.computeShaderPath = "Shaders/particleUpdate.spv";
	computePassCreateInfo.extent = { MAX_PARTICLE_COUNT, 1 };
	computePassCreateInfo.dispatchGroups = { 16, 1, 1 };

	DescriptorSetGenerator descriptorSetGenerator;
	descriptorSetGenerator.addBuffer(particlesBuffer, VK_SHADER_STAGE_COMPUTE_BIT, 0);
	descriptorSetGenerator.addUniformBuffer(frameUniformBuffer, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	computePassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

	int computePassID = scene->addComputePass(computePassCreateInfo);

	// Render Pass Creation
	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.commandBufferID = -1; // default command buffer
	renderPassCreateInfo.outputIsSwapChain = true;
	int renderPassID = scene->addRenderPass(renderPassCreateInfo);

	// Load triangle
	std::vector<Vertex2DTextured> vertices =
	{
		{ glm::vec2(-1.0f, -1.0f), glm::vec2(0.0, 0.0) }, // top left
		{ glm::vec2(1.0f, -1.0f), glm::vec2(1.0, 0.0) }, // top right
		{ glm::vec2(-1.0f, 1.0f), glm::vec2(0.0, 1.0) }, // bot left
		{ glm::vec2(1.0f, 1.0f), glm::vec2(1.0, 1.0) } // bot right
	};

	std::vector<uint32_t> indices =
	{
		0, 1, 2,
		1, 3, 2
	};

	Model::ModelCreateInfo modelCreateInfo{};
	modelCreateInfo.inputVertexTemplate = InputVertexTemplate::POSITION_TEXTURECOORD_2D;
	Model* model = instance.createModel(modelCreateInfo); 
	model->addMeshFromVertices(vertices.data(), vertices.size(), sizeof(Vertex2D), indices); // data are pushed to GPU here

	Instance<ParticleInfo>* instanceBuffer = instance.createInstanceBuffer<ParticleInfo>();
	instanceBuffer->createFromBuffer(particlesBuffer);

	// Renderer creation (define the pipeline)
	RendererCreateInfo rendererCreateInfo{};
	rendererCreateInfo.renderPassID = renderPassID;
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::POSITION_TEXTURECOORD_2D;
	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO; // no instance template

	std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions = ParticleInfo::getAttributeDescriptions(1, 2);
	std::vector<VkVertexInputBindingDescription> inputBindingDescriptions = { ParticleInfo::getBindingDescription(1) };

	for (VkVertexInputAttributeDescription& inputAttributeDescription : inputAttributeDescriptions)
		rendererCreateInfo.pipelineCreateInfo.vertexInputAttributeDescriptions.push_back(inputAttributeDescription);
	for (VkVertexInputBindingDescription& inputBindingDescription : inputBindingDescriptions)
		rendererCreateInfo.pipelineCreateInfo.vertexInputBindingDescriptions.push_back(inputBindingDescription);

	ShaderCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.filename = "Shaders/vert.spv";
	vertexShaderCreateInfo.entryPointName = "main"; // not necessary as this is the default option
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

	ShaderCreateInfo fragmentShaderCreateInfo{};
	fragmentShaderCreateInfo.filename = "Shaders/frag.spv";
	fragmentShaderCreateInfo.entryPointName = "main"; // not necessary as this is the default option
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);

	rendererCreateInfo.pipelineCreateInfo.extent = { WINDOW_WIDTH, WINDOW_HEIGHT };
	rendererCreateInfo.pipelineCreateInfo.alphaBlending = { true };

	// Create the texture
	Image* image = instance.createImageFromFile("Textures/heart_PNG51337.png");
	Sampler* sampler = instance.createSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 1.0f, VK_FILTER_LINEAR);

	DescriptorSetGenerator renderingDescriptorSetGenerator{};
	renderingDescriptorSetGenerator.addCombinedImageSampler(image, sampler, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	renderingDescriptorSetGenerator.addBuffer(nullptr, VK_SHADER_STAGE_FRAGMENT_BIT, 8);

	rendererCreateInfo.descriptorLayouts = renderingDescriptorSetGenerator.getDescriptorLayouts();

	int rendererID = scene->addRenderer(rendererCreateInfo);

	// Link the model to the renderer
	Renderer::AddMeshInfo addMeshInfo{};
	addMeshInfo.renderPassID = renderPassID;
	addMeshInfo.rendererID = rendererID;
	addMeshInfo.vertexBuffer = model->getVertexBuffers()[0];
	addMeshInfo.instanceBuffer = { particlesBuffer->getBuffer(), MAX_PARTICLE_COUNT };
	addMeshInfo.descriptorSetCreateInfo = renderingDescriptorSetGenerator.getDescritorSetCreateInfo();

	scene->addMesh(addMeshInfo);

	// Text
	// Renderer creation (define the pipeline)
	rendererCreateInfo = {};
	rendererCreateInfo.renderPassID = renderPassID;
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::POSITION_TEXTURECOORD_ID_2D;
	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO; // no instancing

	vertexShaderCreateInfo.filename = "Shaders/text/vert.spv";
	vertexShaderCreateInfo.entryPointName = "main"; // not necessary as this is the default option
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(vertexShaderCreateInfo);

	fragmentShaderCreateInfo.filename = "Shaders/text/frag.spv";
	fragmentShaderCreateInfo.entryPointName = "main"; // not necessary as this is the default option
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	rendererCreateInfo.pipelineCreateInfo.shaderCreateInfos.push_back(fragmentShaderCreateInfo);

	rendererCreateInfo.pipelineCreateInfo.extent = { WINDOW_WIDTH, WINDOW_HEIGHT };
	rendererCreateInfo.pipelineCreateInfo.alphaBlending = { true };

	// Font loading
	Font* font = instance.createFont(48, "Fonts/arial.ttf");

	// Uniform Buffer
	DescriptorLayout uniformBufferLayout;
	uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferLayout.accessibility = VK_SHADER_STAGE_VERTEX_BIT;
	uniformBufferLayout.binding = 0;

	// Images
	DescriptorLayout imageLayout;
	imageLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	imageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
	imageLayout.binding = 2;
	imageLayout.count = font->getImages().size();

	// Sampler
	DescriptorLayout samplerLayout;
	samplerLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayout.binding = 1;

	rendererCreateInfo.descriptorLayouts = { uniformBufferLayout, imageLayout, samplerLayout };

	int textRendererID = scene->addRenderer(rendererCreateInfo);

	// Add text
	Scene::AddTextInfo addTextInfo;
	addTextInfo.renderPassID = renderPassID;
	addTextInfo.rendererID = textRendererID;
	addTextInfo.size = 0.05f;

	Text* text = instance.createText();
	int textID = text->addWString(L"This is text", glm::vec2(-0.97f, 0.85f), glm::vec3(1.0f));

	addTextInfo.font = font;
	addTextInfo.text = text;

	scene->addText(addTextInfo);

	// Record
	scene->record();

	// Initialize timer
	auto start = std::chrono::high_resolution_clock::now();
	auto frameStart = std::chrono::high_resolution_clock::now();
	auto fpsCounterStart = std::chrono::high_resolution_clock::now();
	int fpsCounter = 0;

	// Main loop
	while (!instance.windowShouldClose())
	{
		fpsCounter++;

		// Update UB
		frameUniformBufferData.fullTime = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count()) / 2000.0f;
		frameUniformBufferData.frameTime = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frameStart).count()) / 2000.0f;
		frameUniformBuffer->updateData(&frameUniformBufferData);
		frameStart = std::chrono::high_resolution_clock::now();

		if (static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - fpsCounterStart).count()) > 1000.0f)
		{
			fpsCounterStart = std::chrono::high_resolution_clock::now();
			text->updateWString(textID, L"FPS: " + std::to_wstring(fpsCounter));
			text->build({ WINDOW_WIDTH, WINDOW_HEIGHT }, font, 0.1f);

			rendererCreateInfo.forceRendererID = textRendererID;
			scene->addRenderer(rendererCreateInfo);
			scene->addText(addTextInfo);
			scene->record();

			fpsCounter = 0;
		}

		// Draw the scene
		instance.frame(scene, { computePassCreateInfo.commandBufferID }, { {computePassCreateInfo.commandBufferID, -1} });
	}

	instance.waitIdle();

	return EXIT_SUCCESS;
}