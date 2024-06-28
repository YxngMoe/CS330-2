///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
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

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		// find the defined material that matches the tag
		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			// pass the material properties into the shader
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/drywall.jpg",
		"drywall");

	bReturn = CreateGLTexture(
		"textures/backdrop.jpg",
		"backdrop");

	bReturn = CreateGLTexture(
		"textures/abstract.jpg",
		"abstract");

	bReturn = CreateGLTexture(
		"textures/stainedglass.jpg",
		"stainedglass");

	bReturn = CreateGLTexture(
		"textures/pyramid.jpg",
		"pyramid");

	bReturn = CreateGLTexture(
		"textures/pyramid2.jpg",
		"pyramid2");

	bReturn = CreateGLTexture(
		"textures/sand.jpg",
		"sand");



	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

void SceneManager::DefineObjectMaterials()
{
	/*setting steel material to use as material for different shapes*/
	OBJECT_MATERIAL steelMaterial;

	steelMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	steelMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	steelMaterial.shininess = 64.0;
	steelMaterial.tag = "steel";

	m_objectMaterials.push_back(steelMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()

{

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("globalAmbientColor", 0.05f, 0.04f, 0.07f);



	/*light source 1*/

	m_pShaderManager->setVec3Value("lightSources[0].position", -5.0f, 5.0f, 10.0f);

	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.7f, 0.1f, 0.05f);

	m_pShaderManager->setVec3Value("lightSources[0].specularColor", .5f, 0.01f, 0.005f);

	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 16.0f);

	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.15f);


	/*light source 2*/

	m_pShaderManager->setVec3Value("lightSources[1].position", 5.0f, 15.0f, 6.0f);

	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.4f, 0.4f, 0.4f);

	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.25f, 0.25f, 0.25f);

	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 8.0f);

	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{

	LoadSceneTextures();

	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();


}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/


void SceneManager::RenderScene()
{
	// Set scale and position for the ground mesh
	glm::vec3 scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("sand");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawPlaneMesh();

	// Draw descending cubes with varying heights and colors
	int numCubes = 9;
	float maxHeight = 3.0f;
	float cubeHeight = maxHeight / numCubes;
	float cubeSize = 0.5f;

	for (int i = 0; i < numCubes; ++i)
	{
		float scaleFactor = static_cast<float>(numCubes - i) / numCubes;
		float yPos = cubeHeight * i;
		float r = 0.8f + (0.2f * scaleFactor);
		float g = 0.6f + (0.4f * scaleFactor);
		float b = 0.4f + (0.6f * scaleFactor);

		scaleXYZ = glm::vec3(scaleFactor * 3.0f * cubeSize, scaleFactor * 3.0f * cubeSize, scaleFactor * 3.0f * cubeSize);
		positionXYZ = glm::vec3(3.0f, yPos, 3.8f);
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(r, g, b, 1.0f);
		SetShaderTexture("pyramid2");
		SetTextureUVScale(1.0, 1.0);
		m_basicMeshes->DrawBoxMesh();
	}

	// Draw ascending cubes and cones with decreasing sizes and varying textures
	int numCubess = 5;
	for (int i = 0; i < numCubess; ++i)
	{
		float scaleFactor = static_cast<float>(numCubess - i) / numCubess;
		float yPos = cubeHeight * i;
		float r = 0.8f + (0.2f * scaleFactor);
		float g = 0.6f + (0.4f * scaleFactor);
		float b = 0.4f + (0.6f * scaleFactor);

		scaleXYZ = glm::vec3(scaleFactor * 3.0f * cubeSize, scaleFactor * 3.0f * cubeSize, scaleFactor * 3.0f * cubeSize);
		positionXYZ = glm::vec3(2.0f, yPos, 5.6f);
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(r, g, b, 1.0f);

		if (i == numCubes - 1) {
			// Skip drawing for the top cube
		}
		else if (i == numCubes - 2) {
			SetShaderTexture("pyramid2");
			SetTextureUVScale(1.0, 1.0);
			m_basicMeshes->DrawConeMesh();
		}
		else {
			SetShaderTexture("pyramid2");
			SetTextureUVScale(1.0, 1.0);
			m_basicMeshes->DrawBoxMesh();
		}
	}

	// Draw large pyramid #2
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);
	positionXYZ = glm::vec3(10.0f, 0.0f, 1.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pyramid");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawConeMesh();

	// Draw medium pyramid #3
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f);
	positionXYZ = glm::vec3(6.0f, 0.0f, 2.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pyramid");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawConeMesh(true); // Use different texture on sides

	// Draw small pyramid #3
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);
	positionXYZ = glm::vec3(6.0f, 1.6f, 2.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pyramid2");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawConeMesh(true);

	// Draw complex texture shape with top and sides using different textures
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);
	positionXYZ = glm::vec3(-1.2f, 0.0f, 4.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("pyramid2");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawCylinderMesh(false, true, true); // Top and sides with different textures

	SetShaderMaterial("steel"); // Set material for subsequent drawing

	SetShaderTexture("sand");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawCylinderMesh(true, false, false); // Draw cylinder with sand texture

	// Draw a small cone on top of the complex shape
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);
	positionXYZ = glm::vec3(0.3f, 0.0f, 3.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.82f, 0.71f, 0.55f, 1.0f);
	m_basicMeshes->DrawConeMesh();
}

