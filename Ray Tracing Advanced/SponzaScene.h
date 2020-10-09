#pragma once

#include <WolfEngine.h>

#include "Camera.h"

class SponzaScene
{
public:
	SponzaScene(Wolf::WolfInstance* wolfInstance);

	void update();

	Wolf::Scene* getScene() const { return m_scene; }

private:
	Camera m_camera;
	GLFWwindow* m_window;

	Wolf::Scene* m_scene = nullptr;

	// Data
	glm::mat4 m_modelMatrix;

	struct UniformBufferCameraPropertiesData
	{
		glm::mat4 viewInverse;
		glm::mat4 projInverse;
		glm::mat4 model;
		glm::mat4 view;
		glm::vec4 position;
	};
	UniformBufferCameraPropertiesData m_uniformBufferCameraPropertiesData;
	Wolf::UniformBuffer* m_uniformBufferCameraProperties;

	struct UniformBufferLightProperties
	{
		glm::vec4 directionDirectionalLight;
		glm::vec4 colorDirectionalLight;
	};
	UniformBufferLightProperties m_uniformBufferLightPropertiesData;
	Wolf::UniformBuffer* m_uniformBufferLightProperties;
};

