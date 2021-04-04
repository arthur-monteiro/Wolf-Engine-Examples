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
	instanceCreateInfo.applicationName = "Ray Tracing simple triangle";
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
	sceneCreateInfo.swapChainCommandType = Scene::CommandType::RAY_TRACING;
	Scene* scene = instance.createScene(sceneCreateInfo); // the scene will be deleted with the wolf instance

	// Load triangle
	std::vector<Vertex2D> vertices =
	{
		{ glm::vec2(0.0f, -0.75f) }, // top
		{ glm::vec2(-0.75f, 0.75f) }, // bot left
		{ glm::vec2(0.75f, 0.75f) } // bot right
	};

	std::vector<uint32_t> indices =
	{
		0, 1, 2
	};

	Model::ModelCreateInfo modelCreateInfo{};
	modelCreateInfo.inputVertexTemplate = InputVertexTemplate::POSITION_2D;
	Model* model = instance.createModel(modelCreateInfo);
	model->addMeshFromVertices(vertices.data(), vertices.size(), sizeof(Vertex2D), indices); // data are pushed to GPU here

	// Acceleration structure creation
	BottomLevelAccelerationStructure::GeometryInfo geometryInfo;
	geometryInfo.vertexBuffer = model->getVertexBuffers()[0];
	geometryInfo.vertexSize = sizeof(Vertex2D);
	geometryInfo.transform = glm::mat4(1.0);
	geometryInfo.transformBuffer = VK_NULL_HANDLE;
	geometryInfo.transformOffsetInBytes = 0;
	geometryInfo.vertexFormat = VK_FORMAT_R32G32_SFLOAT;
	
	AccelerationStructure* accelerationStructure = instance.createAccelerationStructure({ geometryInfo });

	// Ray Tracing Pass creation
	Scene::RayTracingPassAddInfo rayTracingPassAddInfo;
	//rayTracingPassAddInfo.extent = { WINDOW_WIDTH, WINDOW_HEIGHT };
	rayTracingPassAddInfo.commandBufferID = -1;
	rayTracingPassAddInfo.outputIsSwapChain = true;
	rayTracingPassAddInfo.outputBinding = 1;

	RayTracingPass::RayTracingPassCreateInfo rayTracingPassCreateInfo;
	rayTracingPassCreateInfo.raygenShader = "Shaders/rgen.spv";
	rayTracingPassCreateInfo.missShaders = { "Shaders/rmiss.spv" };

	RayTracingPass::RayTracingPassCreateInfo::HitGroup hitGroup;
	hitGroup.closestHitShader = "Shaders/rchit.spv";

 	RayTracingPass::RayTracingPassCreateInfo::HitGroup hitGroup2;
 	hitGroup2.closestHitShader = "Shaders/rchit2.spv";

	rayTracingPassCreateInfo.hitGroups = { hitGroup, hitGroup2 };

	// Descriptor set
	DescriptorSetGenerator descriptorSetGenerator;
	descriptorSetGenerator.addAccelerationStructure(accelerationStructure, VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 0);
	descriptorSetGenerator.addBuffer(model->getVertexBuffers()[0].vertexBuffer, VK_WHOLE_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 2);
	descriptorSetGenerator.addBuffer(model->getVertexBuffers()[0].indexBuffer, VK_WHOLE_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 3);

	rayTracingPassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();
	rayTracingPassAddInfo.rayTracingPassCreateInfo = rayTracingPassCreateInfo;

	int rayTracingPassID = scene->addRayTracingPass(rayTracingPassAddInfo);

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