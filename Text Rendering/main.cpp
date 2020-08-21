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

	// Renderer creation (define the pipeline)
	RendererCreateInfo rendererCreateInfo{};
	rendererCreateInfo.renderPassID = renderPassID;
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::POSITION_TEXTURECOORD_ID_2D;
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

	int rendererID = scene->addRenderer(rendererCreateInfo);

	// Add text
	Scene::AddTextInfo addTextInfo;
	addTextInfo.renderPassID = renderPassID;
	addTextInfo.rendererID = rendererID;
	addTextInfo.size = 0.1f;

	Text* text = instance.createText();
	text->addWString(L"This is text", glm::vec2(0.0f), glm::vec3(1.0f));

	addTextInfo.font = font;
	addTextInfo.text = text;
	
	scene->addText(addTextInfo);

	// Record
	scene->record();

	// Main loop
	while (!instance.windowShouldClose())
	{
		// Draw the scene
		instance.frame(scene, {}, {});
	}

	instance.waitIdle();

	return EXIT_SUCCESS;
}