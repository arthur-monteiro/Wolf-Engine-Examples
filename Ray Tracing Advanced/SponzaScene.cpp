#include "SponzaScene.h"

using namespace Wolf;

SponzaScene::SponzaScene(Wolf::WolfInstance* wolfInstance)
{
	m_window = wolfInstance->getWindowPtr();

	// Scene creation
	Scene::SceneCreateInfo sceneCreateInfo;
	sceneCreateInfo.swapChainCommandType = Scene::CommandType::RAY_TRACING;

	m_scene = wolfInstance->createScene(sceneCreateInfo);

	m_camera.initialize(glm::vec3(1.4f, 1.2f, 0.3f), glm::vec3(2.0f, 0.9f, -0.3f), glm::vec3(0.0f, 1.0f, 0.0f), 0.01f, 5.0f,
		16.0f / 9.0f);

	// Data
	m_modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
	
	m_uniformBufferCameraPropertiesData.projInverse = glm::inverse(m_camera.getProjection());
	m_uniformBufferCameraPropertiesData.model = m_modelMatrix;
	m_uniformBufferCameraProperties = wolfInstance->createUniformBufferObject(&m_uniformBufferCameraPropertiesData, sizeof(UniformBufferCameraPropertiesData));

	m_uniformBufferLightPropertiesData.directionDirectionalLight = glm::vec4(4.0f, -5.0f, -1.5f, 0.0f);
	m_uniformBufferLightPropertiesData.colorDirectionalLight = glm::vec4(10.0f, 9.0f, 6.0f, 1.0f);
	m_uniformBufferLightProperties = wolfInstance->createUniformBufferObject(&m_uniformBufferLightPropertiesData, sizeof(UniformBufferLightProperties));

	// Model loading
	Model::ModelCreateInfo modelCreateInfo{};
	modelCreateInfo.inputVertexTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	Model* model = wolfInstance->createModel<>(modelCreateInfo);

	Model::ModelLoadingInfo modelLoadingInfo;
	modelLoadingInfo.filename = "Models/sponza/sponza.obj";
	modelLoadingInfo.mtlFolder = "Models/sponza";
	model->loadObj(modelLoadingInfo);

	// Acceleration structure creation
	BottomLevelAccelerationStructure::GeometryInfo geometryInfo;
	geometryInfo.vertexBuffer = model->getVertexBuffers()[0];
	geometryInfo.vertexSize = sizeof(Vertex3D);
	geometryInfo.transform = m_modelMatrix;
	geometryInfo.transformBuffer = VK_NULL_HANDLE;
	geometryInfo.transformOffsetInBytes = 0;
	geometryInfo.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;

	AccelerationStructure* accelerationStructure = wolfInstance->createAccelerationStructure({ geometryInfo });

	// Ray Tracing Pass creation
	Scene::RayTracingPassAddInfo rayTracingPassAddInfo;
	rayTracingPassAddInfo.extent = { 0, 0 };
	rayTracingPassAddInfo.commandBufferID = -1;
	rayTracingPassAddInfo.outputIsSwapChain = true;
	rayTracingPassAddInfo.outputBinding = 1;

	RayTracingPass::RayTracingPassCreateInfo rayTracingPassCreateInfo;
	rayTracingPassCreateInfo.raygenShader = "Shaders/rgen.spv";
	rayTracingPassCreateInfo.missShaders = { "Shaders/rmiss.spv", "Shaders/shadowRMiss.spv" };

	RayTracingPass::RayTracingPassCreateInfo::HitGroup hitGroup;
	hitGroup.closestHitShader = "Shaders/rchit.spv";
	
	rayTracingPassCreateInfo.hitGroups = { hitGroup };

	// Descriptor set
	DescriptorSetGenerator descriptorSetGenerator;
	descriptorSetGenerator.addAccelerationStructure(accelerationStructure, VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 0);
	descriptorSetGenerator.addUniformBuffer(m_uniformBufferCameraProperties, VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 2);
	descriptorSetGenerator.addBuffer(model->getVertexBuffers()[0].vertexBuffer, VK_WHOLE_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 3);
	descriptorSetGenerator.addBuffer(model->getVertexBuffers()[0].indexBuffer, VK_WHOLE_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 4);
	descriptorSetGenerator.addUniformBuffer(m_uniformBufferLightProperties, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 5);
	descriptorSetGenerator.addSampler(model->getSampler(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 6);
	descriptorSetGenerator.addImages(model->getImages(), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 7);

	rayTracingPassCreateInfo.descriptorSetCreateInfo = descriptorSetGenerator.getDescritorSetCreateInfo();
	rayTracingPassAddInfo.rayTracingPassCreateInfo = rayTracingPassCreateInfo;

	int rayTracingPassID = m_scene->addRayTracingPass(rayTracingPassAddInfo);

	m_scene->record();
}

void SponzaScene::update()
{
	m_camera.update(m_window);
	
	m_uniformBufferCameraPropertiesData.viewInverse = glm::inverse(m_camera.getViewMatrix());
	m_uniformBufferCameraPropertiesData.view = m_camera.getViewMatrix();
	m_uniformBufferCameraPropertiesData.position = glm::vec4(m_camera.getPosition(), 0.0f);
	m_uniformBufferCameraProperties->updateData(&m_uniformBufferCameraPropertiesData);
}