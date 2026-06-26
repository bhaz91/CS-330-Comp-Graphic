///////////////////////////////////////////////////////////////////////////////
// Brandon Hazelton
// CS 330 Final Project
// SceneManager.cpp
// ================
// Loads textures, sets up lighting, and renders a complete 3D mug scene.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <iostream>
#include <glm/gtx/transform.hpp>

namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}

	m_loadedTextures = 0;
}

SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;

	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

	DestroyGLTextures();
}

bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);

	if (image)
	{
		std::cout << "Successfully loaded image: " << filename
			<< ", width: " << width
			<< ", height: " << height
			<< ", channels: " << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (colorChannels == 3)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		}
		else if (colorChannels == 4)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		}
		else
		{
			std::cout << "Texture has unsupported channel count: " << colorChannels << std::endl;
			stbi_image_free(image);
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);

		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image: " << filename << std::endl;
	return false;
}

void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

int SceneManager::FindTextureID(std::string tag)
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		if (m_textureIDs[i].tag.compare(tag) == 0)
		{
			return m_textureIDs[i].ID;
		}
	}

	return -1;
}

int SceneManager::FindTextureSlot(std::string tag)
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		if (m_textureIDs[i].tag.compare(tag) == 0)
		{
			return i;
		}
	}

	return -1;
}

bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	for (int i = 0; i < m_objectMaterials.size(); i++)
	{
		if (m_objectMaterials[i].tag.compare(tag) == 0)
		{
			material = m_objectMaterials[i];
			return true;
		}
	}

	return false;
}

void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	scale = glm::scale(scaleXYZ);
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		int textureSlot = FindTextureSlot(textureTag);

		if (textureSlot >= 0)
		{
			m_pShaderManager->setIntValue(g_UseTextureName, true);
			m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
		}
		else
		{
			m_pShaderManager->setIntValue(g_UseTextureName, false);
		}
	}
}

void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

void SceneManager::SetShaderMaterial(std::string materialTag)
{
	if (NULL != m_pShaderManager)
	{
		OBJECT_MATERIAL material;

		if (FindMaterial(materialTag, material))
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// White point light above the scene.
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 10.0f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 1.0f);

	// Warm colored side light.
	m_pShaderManager->setVec3Value("lightSources[1].position", -6.0f, 5.0f, 2.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.15f, 0.10f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 1.0f, 0.55f, 0.20f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 0.65f, 0.30f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 24.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.60f);
}

void SceneManager::LoadSceneTextures()
{
	CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "wood");
	CreateGLTexture("../../Utilities/textures/tilesf2.jpg", "tile");
	CreateGLTexture("../../Utilities/textures/stainless.jpg", "metal");

	BindGLTextures();
}

void SceneManager::PrepareScene()
{
	LoadSceneTextures();
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadTorusMesh();
}

void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;

	// -------------------------------------------------------
	// Object 1: Wood floor plane
	// -------------------------------------------------------
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");
	SetTextureUVScale(6.0f, 4.0f);
	m_basicMeshes->DrawPlaneMesh();

	// -------------------------------------------------------
	// Object 2: Back wall
	// -------------------------------------------------------
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	positionXYZ = glm::vec3(0.0f, 9.0f, -10.0f);

	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.80f, 0.80f, 0.80f, 1.0f);
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// -------------------------------------------------------
	// Object 3: Mug body
	// -------------------------------------------------------
	scaleXYZ = glm::vec3(2.0f, 3.5f, 2.0f);
	positionXYZ = glm::vec3(0.0f, 2.0f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("tile");
	SetTextureUVScale(3.0f, 3.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Mug handle
	scaleXYZ = glm::vec3(0.8f, 1.2f, 0.3f);
	positionXYZ = glm::vec3(2.4f, 2.0f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// Mug top rim
	scaleXYZ = glm::vec3(2.1f, 2.1f, 0.2f);
	positionXYZ = glm::vec3(0.0f, 3.8f, 0.0f);

	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// Mug base
	scaleXYZ = glm::vec3(1.7f, 0.2f, 1.7f);
	positionXYZ = glm::vec3(0.0f, 0.2f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// -------------------------------------------------------
	// Object 4: Plate under the mug
	// -------------------------------------------------------
	scaleXYZ = glm::vec3(4.3f, 0.15f, 4.3f);
	positionXYZ = glm::vec3(0.0f, 0.10f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.95f, 0.95f, 0.88f, 1.0f);
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Plate rim
	scaleXYZ = glm::vec3(4.4f, 4.4f, 0.12f);
	positionXYZ = glm::vec3(0.0f, 0.28f, 0.0f);

	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.82f, 0.82f, 0.75f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// -------------------------------------------------------
	// Object 5: Metal spoon beside the mug
	// -------------------------------------------------------
	scaleXYZ = glm::vec3(0.12f, 0.12f, 3.5f);
	positionXYZ = glm::vec3(-3.5f, 0.35f, 0.0f);

	SetTransformations(scaleXYZ, 90.0f, 0.0f, 25.0f, positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Spoon bowl
	scaleXYZ = glm::vec3(0.65f, 0.18f, 1.0f);
	positionXYZ = glm::vec3(-4.2f, 0.35f, 1.4f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 25.0f, positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// -------------------------------------------------------
	// Object 6: Sugar cubes
	// -------------------------------------------------------
	scaleXYZ = glm::vec3(0.7f, 0.7f, 0.7f);
	positionXYZ = glm::vec3(3.5f, 0.45f, -1.0f);

	SetTransformations(scaleXYZ, 0.0f, 20.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 0.95f, 1.0f);
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.7f, 0.7f, 0.7f);
	positionXYZ = glm::vec3(4.2f, 0.45f, -0.5f);

	SetTransformations(scaleXYZ, 0.0f, -15.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 0.95f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.65f, 0.65f, 0.65f);
	positionXYZ = glm::vec3(3.8f, 1.15f, -0.7f);

	SetTransformations(scaleXYZ, 0.0f, 8.0f, 0.0f, positionXYZ);
	SetShaderColor(0.98f, 0.98f, 0.92f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// -------------------------------------------------------
	// Object 7: Steam rising from mug
	// -------------------------------------------------------
	scaleXYZ = glm::vec3(0.18f, 1.2f, 0.18f);
	positionXYZ = glm::vec3(-0.6f, 5.0f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, -20.0f, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.85f, 0.55f);
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.18f, 1.3f, 0.18f);
	positionXYZ = glm::vec3(0.0f, 5.25f, 0.2f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 15.0f, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.85f, 0.55f);
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.18f, 1.1f, 0.18f);
	positionXYZ = glm::vec3(0.6f, 5.0f, -0.1f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, -15.0f, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.85f, 0.55f);
	m_basicMeshes->DrawCylinderMesh();

	// -------------------------------------------------------
	// Object 8: Small cone decoration
	// -------------------------------------------------------
	scaleXYZ = glm::vec3(0.8f, 1.4f, 0.8f);
	positionXYZ = glm::vec3(-5.0f, 0.7f, -2.5f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.40f, 0.25f, 0.12f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	// -------------------------------------------------------
	// Object 9: Small pyramid decoration
	// -------------------------------------------------------
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);
	positionXYZ = glm::vec3(5.0f, 0.55f, -2.2f);

	SetTransformations(scaleXYZ, 0.0f, 45.0f, 0.0f, positionXYZ);
	SetShaderColor(0.25f, 0.35f, 0.55f, 1.0f);
	m_basicMeshes->DrawPyramid4Mesh();
}