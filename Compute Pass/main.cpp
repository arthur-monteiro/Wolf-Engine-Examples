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
	sceneCreateInfo.swapChainCommandType = Scene::CommandType::COMPUTE;
	Scene* scene = instance.createScene(sceneCreateInfo); // the scene will be deleted with the wolf instance

	// Compute pass creation
	Scene::ComputePassCreateInfo computePassCreateInfo;
	computePassCreateInfo.commandBufferID = -1;
	computePassCreateInfo.outputIsSwapChain = true;
	computePassCreateInfo.computeShaderPath = "Shaders/comp.spv";
	computePassCreateInfo.dispatchGroups = { 16, 16, 1 };
	computePassCreateInfo.outputBinding = 0;

	int computePassID = scene->addComputePass(computePassCreateInfo);

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