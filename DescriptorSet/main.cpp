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

int main()
{
	WolfInstanceCreateInfo instanceCreateInfo;

	// Application
	instanceCreateInfo.applicationName = "This wil be the title of the window";
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

	// Render Pass Creation
	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.commandBufferID = -1; // default command buffer
	renderPassCreateInfo.outputIsSwapChain = true;
	int renderPassID = scene->addRenderPass(renderPassCreateInfo);

	// Load triangle
	std::vector<Vertex2DTextured> vertices =
	{
		{ glm::vec2(0.0f, -0.75f), glm::vec2(0.5f, 0.0f) }, // top
		{ glm::vec2(-0.75f, 0.75f), glm::vec2(0.0f, 1.0f) }, // bot left
		{ glm::vec2(0.75f, 0.75f),glm::vec2(1.0f, 1.0f) } // bot right
	};

	std::vector<uint32_t> indices =
	{
		0, 1, 2
	};

	Model::ModelCreateInfo modelCreateInfo{};
	modelCreateInfo.inputVertexTemplate = InputVertexTemplate::POSITION_TEXTURECOORD_2D;
	Model* model = instance.createModel(modelCreateInfo); 
	model->addMeshFromVertices(vertices.data(), vertices.size(), sizeof(Vertex2D), indices); // data are pushed to GPU here

	// Renderer creation (define the pipeline)
	RendererCreateInfo rendererCreateInfo{};
	rendererCreateInfo.renderPassID = renderPassID;
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::POSITION_TEXTURECOORD_2D;
	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO; // no instancing

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
	rendererCreateInfo.pipelineCreateInfo.alphaBlending = { false };

	// Create the descriptor set (will generate layout and descriptor set)
	// Create the uniform buffer
	float offset = 0.0f;
	UniformBuffer* uniformBuffer = instance.createUniformBufferObject(&offset, sizeof(float));

	// Create the texture
	Image* image = instance.createImageFromFile("Images/texture.jpg");
	Sampler* sampler = instance.createSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, 1.0f, VK_FILTER_LINEAR);

	// Generate the descriptor set definition
	DescriptorSetGenerator descriptorSetGenerator{};
	descriptorSetGenerator.addUniformBuffer(uniformBuffer, VK_SHADER_STAGE_VERTEX_BIT, 0);
	descriptorSetGenerator.addCombinedImageSampler(image, sampler, VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	// Get layouts
	rendererCreateInfo.descriptorLayouts = descriptorSetGenerator.getDescriptorLayouts();

	int rendererID = scene->addRenderer(rendererCreateInfo);

	// Link the model to the renderer
	Renderer::AddMeshInfo addMeshInfo{};
	addMeshInfo.renderPassID = renderPassID;
	addMeshInfo.rendererID = rendererID;
	addMeshInfo.vertexBuffer = model->getVertexBuffers()[0];

	// Get descriptor set definition
	addMeshInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();

	scene->addMesh(addMeshInfo);

	// Record
	scene->record();

	// Initialize timer
	auto start = std::chrono::high_resolution_clock::now();

	// Main loop
	while (!instance.windowShouldClose())
	{
		// Update the uniform buffer
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
		offset = glm::sin(static_cast<float>(duration)/ 1000.0f);
		uniformBuffer->updateData(&offset);
		
		// Draw the scene
		instance.frame(scene, {}, {});
	}

	instance.waitIdle();

	return EXIT_SUCCESS;
}