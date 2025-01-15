#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

#include <iostream>
#include "SkyBox.hpp"

// Render modes

enum RenderingMode {
	SOLID,
	WIREFRAME,
	POINTS,
	SMOOTH
};
RenderingMode currentRenderingMode = SMOOTH;
float debounceDelay = 0.3f;
bool canSwitchRenderMode = true;

// Window

gps::Window myWindow;

// Pressed keys array

GLboolean pressedKeys[1024];

// Matrices

glm::mat4 airportModelMatrix, airplaneModelMatrix, lamp1ModelMatrix, propellerModelMatrix, modelMatrix;
glm::mat3 airportNormalMatrix, airplaneNormalMatrix, lamp1NormalMatrix, normalMatrix;
glm::mat4 view;
glm::mat4 projection;
glm::vec3 orbitCenter = glm::vec3(25.0f, 50.0f, 25.0f);
int propellerId = 27;
float propellerRotationAngle = 0.0f;
float airplaneOrbitAngle = 0.0f;
float airplaneOrbitSpeed = 0.5f;
float orbitRadius = 100.0f;

// Uniform Locations

GLint airportModelLoc, airplaneModelLoc, lamp1ModelLoc;
GLint airportNormalMatrixLoc, airplaneNormalMatrixLoc, lamp1NormalMatrixLoc;
GLint viewLoc;
GLint projectionLoc;

// Global light

glm::vec3 globalLightDir = glm::vec3(0.0f, 25.0f, 0.0f);
glm::vec3 globalLightColor = glm::vec3(0.3f, 0.3f, 0.3f);
GLuint globalLightDirLoc;
GLuint globalLightColorLoc;
float globalLightMoveSpeed = 0.75f;

// Lamp light

glm::vec3 lightPosition;
glm::vec3 lightDir;
glm::vec3 lightColor;
GLint lightPositionLoc;
GLint lightDirLoc;
GLint lightColorLoc;
float currentTime = 0.0f;

// Airplane light

glm::vec3 light2Position;
glm::vec3 light2Direction;
glm::vec3 light2Color;
GLint light2PositionLoc;
GLint light2DirectionLoc;
GLint light2ColorLoc;

// Camera

gps::Camera myCamera(
	glm::vec3(0.0f, 25.0f, -15.0f),
	glm::vec3(0.0f, 20.0f, 15.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
GLfloat cameraSpeed = 0.75f;

// 3D Models

gps::Model3D airport, airplane, lamp1, lightCube, screenQuad;

// Shaders

gps::Shader myBasicShader, skyboxShader, depthMapShader, screenQuadShader, lightShader;

// Skybox

gps::SkyBox skyBox;
bool isDay = false;
float ambientStrength = 0.0f;

// Fog

glm::vec3 fogColor = glm::vec3(0.1f, 0.1f, 0.2f);
GLint fogColorLoc, fogStartLoc, fogEndLoc;
float fogStart = 0.0f;
float fogEnd = 1000.0f;

// Shadow Map

glm::mat4 lightRotation;
GLfloat lightAngle;
GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
bool showDepthMap = false;

// Mouse

float lastX = 1024.0f;
float lastY = 768.0f; 
float sensitivity = 0.025f;
bool firstMouse = true;
bool enableMouse = false;

// Presentation mode

glm::vec3 presentationCenter = glm::vec3(200.0f, 30.0f, 25.0f);
float presentationAngle = 0.0f; 
float presentationSpeed = 0.25f; 
float presentationRadius = 200.0f;
bool presentationMode = false;

void initFBO() {
	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO 
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//attach texture to FBO 
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	glm::vec3 lightPos = -glm::normalize(globalLightDir) * 25.0f;

	glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	const GLfloat near_plane = 25.0f, far_plane = 100.0f; 
	const GLfloat ortho_size = 25.0f;

	glm::mat4 lightProjection = glm::ortho(-ortho_size, ortho_size, -ortho_size, ortho_size, near_plane, far_plane);

	return lightProjection * lightView;
}

void initSkyBox(bool isDay) {
	skyboxShader.useShaderProgram();
	std::vector<const GLchar*> faces;

	if (!isDay) {
		// Night skybox
		faces.push_back("skybox/posx.png"); // right
		faces.push_back("skybox/negx.png"); // left
		faces.push_back("skybox/posy.png"); // top
		faces.push_back("skybox/negy.png"); // bottom
		faces.push_back("skybox/posz.png"); // front
		faces.push_back("skybox/negz.png"); // back
	}
	else {
		// Day skybox
		faces.push_back("skybox/px.png"); // right
		faces.push_back("skybox/nx.png"); // left
		faces.push_back("skybox/py.png"); // top
		faces.push_back("skybox/ny.png"); // bottom
		faces.push_back("skybox/pz.png"); // front
		faces.push_back("skybox/nz.png"); // back
	}

	skyBox.Load(faces);
}

GLenum glCheckError_(const char* file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
		case GL_INVALID_ENUM:
			error = "INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			error = "INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			error = "INVALID_OPERATION";
			break;
		case GL_OUT_OF_MEMORY:
			error = "OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error = "INVALID_FRAMEBUFFER_OPERATION";
			break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	if (width == 0 || height == 0) {
		return;
	}
	glViewport(0, 0, width, height);

	float aspectRatio = (float) width / height;
	projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 2000.0f);

	myBasicShader.useShaderProgram();
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	myWindow.setWindowDimensions({ width, height });
}

void switchRenderMode(RenderingMode mode) {
	currentRenderingMode = mode;
	myBasicShader.useShaderProgram();
	GLint useFlatShadingLoc = glGetUniformLocation(myBasicShader.shaderProgram, "useFlatShading");

	switch (mode) {
	case SOLID:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUniform1i(useFlatShadingLoc, GL_TRUE);
		glDisable(GL_POLYGON_SMOOTH);
		std::cout << "Switched to SOLID mode." << std::endl;
		break;
	case WIREFRAME:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUniform1i(useFlatShadingLoc, GL_FALSE);
		glDisable(GL_POLYGON_SMOOTH);
		std::cout << "Switched to WIREFRAME mode." << std::endl;
		break;
	case POINTS:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		glUniform1i(useFlatShadingLoc, GL_FALSE);
		glDisable(GL_POLYGON_SMOOTH);
		glPointSize(10.0f);
		std::cout << "Switched to POINTS mode." << std::endl;
		break;
	case SMOOTH:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUniform1i(useFlatShadingLoc, GL_FALSE);
		glEnable(GL_POLYGON_SMOOTH);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		std::cout << "Switched to SMOOTH mode." << std::endl;
		break;
	}
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key == GLFW_KEY_C && action == GLFW_PRESS) {
		enableMouse = !enableMouse;
		glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, enableMouse? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		airplaneOrbitSpeed += 0.1f;
	}

	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		airplaneOrbitSpeed = std::max(0.1f, airplaneOrbitSpeed - 0.1f);
	}

	if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		presentationMode = !presentationMode;
	}

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS) {
			pressedKeys[key] = true;

			if (key == GLFW_KEY_1) {
				switchRenderMode(SOLID);
			}
			else if (key == GLFW_KEY_2) {
				switchRenderMode(WIREFRAME);
			}
			else if (key == GLFW_KEY_3) {
				switchRenderMode(POINTS);
			}
			else if (key == GLFW_KEY_4) {
				switchRenderMode(SMOOTH);
			}

			if (key == GLFW_KEY_N) {
				isDay = !isDay;
				if (isDay) {
					fogStart = 1000.0f;
					fogEnd = 2000.0f;
					fogColor = glm::vec3(1.0f, 1.0f, 1.0f);

					lightColor = glm::vec3(0.0f);
					light2Color = glm::vec3(0.0f);
					ambientStrength = 0.85f;
				}
				else {
					fogStart = 0.0f;
					fogEnd = 1000.0f;
					fogColor = glm::vec3(0.1f, 0.1f, 0.2f);

					lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
					light2Color = glm::vec3(0.5f, 1.0f, 1.0f);
					ambientStrength = 0.15f;
				}

				myBasicShader.useShaderProgram();
				glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "isDay"), isDay);

				glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "fogColor"), 1, glm::value_ptr(fogColor));
				glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogStart"), fogStart);
				glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogEnd"), fogEnd);

				glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
				glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "light2Color"), 1, glm::value_ptr(light2Color));
				glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "ambientStrength"), ambientStrength);

				initSkyBox(isDay);

				std::cout << (isDay ? "Day mode activated." : "Night mode activated.") << std::endl;
			}

			canSwitchRenderMode = false;
			glfwSetTime(0);
		}
		else if (action == GLFW_RELEASE) {
			pressedKeys[key] = false;
		}
	}
}

void updateCamera() {
	view = myCamera.getViewMatrix();
	myBasicShader.useShaderProgram();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	airplaneNormalMatrix = glm::mat3(glm::inverseTranspose(view * airplaneModelMatrix));
	lamp1NormalMatrix = glm::mat3(glm::inverseTranspose(view * lamp1ModelMatrix));
}

void present() {
	if (presentationMode) {
		presentationAngle += presentationSpeed * 0.1f;
		if (presentationAngle >= 360.0f) {
			presentationAngle -= 360.0f;
		}

		float x = presentationCenter.x + presentationRadius * cos(glm::radians(presentationAngle));
		float z = presentationCenter.z + presentationRadius * sin(glm::radians(presentationAngle));
		glm::vec3 cameraPosition = glm::vec3(x, presentationCenter.y, z);
		glm::vec3 cameraTarget = presentationCenter;

		myCamera.setCameraPosition(cameraPosition);
		myCamera.setCameraTarget(cameraTarget);
		updateCamera();
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; 

	lastX = xpos;
	lastY = ypos;

	xoffset *= sensitivity;
	yoffset *= sensitivity;

	if (yoffset > 89.0f)
		yoffset = 89.0f;
	if (yoffset < -89.0f)
		yoffset = -89.0f;

	if (xoffset > 89.0f)
		xoffset = 89.0f;
	if (xoffset < -89.0f)
		xoffset = -89.0f;

	myCamera.rotate(yoffset, xoffset);
	updateCamera();
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		updateCamera();
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
		updateCamera();
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
		updateCamera();
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
		updateCamera();
	}

	if (pressedKeys[GLFW_KEY_UP]) {
		myCamera.rotate(cameraSpeed, 0.0f);
		updateCamera();
	}

	if (pressedKeys[GLFW_KEY_DOWN]) {
		myCamera.rotate(-cameraSpeed, 0.0f);
		updateCamera();
	}

	if (pressedKeys[GLFW_KEY_LEFT]) {
		myCamera.rotate(0.0f, cameraSpeed);
		updateCamera();
	}

	if (pressedKeys[GLFW_KEY_RIGHT]) {
		myCamera.rotate(0.0f, -cameraSpeed);
		updateCamera();
	}

	if (pressedKeys[GLFW_KEY_I]) {
		globalLightDir.z -= globalLightMoveSpeed; // Move light forward
	}

	if (pressedKeys[GLFW_KEY_K]) {
		globalLightDir.z += globalLightMoveSpeed; // Move light backward
	}

	if (pressedKeys[GLFW_KEY_J]) {
		globalLightDir.x -= globalLightMoveSpeed; // Move light left
	}

	if (pressedKeys[GLFW_KEY_L]) {
		globalLightDir.x += globalLightMoveSpeed; // Move light right
	}

	if (pressedKeys[GLFW_KEY_U]) {
		globalLightDir.y += globalLightMoveSpeed; // Move light up
	}

	if (pressedKeys[GLFW_KEY_O]) {
		globalLightDir.y -= globalLightMoveSpeed; // Move light down
	}
}

void updateLightFlicker(float currentTime) {
	float flicker = 0.8f + 0.2f * sin(currentTime * 10.0f) + (rand() % 10) / 50.0f;

	lightColor = glm::vec3(1.0f, 1.0f, 1.0f) * flicker;

	myBasicShader.useShaderProgram();
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void initOpenGLWindow() {
	myWindow.Create(1024, 768, "Airport Scene");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
	glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
	glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
	airport.LoadModel("models/airport/airport.obj");
	airplane.LoadModel("models/airplane/airplane.obj");
	lamp1.LoadModel("models/lamp/StreetLamp.obj");
	lightCube.LoadModel("models/cube/cube.obj");
	screenQuad.LoadModel("models/quad/quad.obj");
}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
}

void initUniforms() {
	myBasicShader.useShaderProgram();

	// Compute model matrices

	airportModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
	airportModelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	airplaneModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, 75.0f, 70.0f));
	airplaneModelMatrix = glm::rotate(airplaneModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	airplaneModelMatrix = glm::rotate(airplaneModelMatrix, glm::radians(-13.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	airplaneModelMatrix = glm::scale(airplaneModelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
	airplaneModelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	lamp1ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(330.0f, 20.0f, 0.0f));
	lamp1ModelMatrix = glm::rotate(lamp1ModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	lamp1ModelMatrix = glm::scale(lamp1ModelMatrix, glm::vec3(10.0f, 10.0f, 10.0f));
	lamp1ModelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// Compute view

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	// Compute normal matrices

	airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	airportNormalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	airplaneNormalMatrix = glm::mat3(glm::inverseTranspose(view * airplaneModelMatrix));
	airplaneNormalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	lamp1NormalMatrix = glm::mat3(glm::inverseTranspose(view * lamp1ModelMatrix));
	lamp1NormalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// Compute projection matrix

	float aspectRatio = (float) myWindow.getWindowDimensions().width / myWindow.getWindowDimensions().height;
	projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 2000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Directional light setup
	
	globalLightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "globalLightDir");
	globalLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "globalLightColor");

	glUniform3fv(globalLightDirLoc, 1, glm::value_ptr(globalLightDir));
	glUniform3fv(globalLightColorLoc, 1, glm::value_ptr(globalLightColor));

	// Point light setup (lamp)

	lightPosition = glm::vec3(330.0f, 60.0f, 5.0f);
	lightColor = glm::vec3(0.0f, 0.0f, 0.0f);
	lightDir = glm::vec3(1.0f, 0.0f, 0.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));

	lightPositionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightPosition");
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");

	glUniform3fv(lightPositionLoc, 1, glm::value_ptr(lightPosition));
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	// Point light setup (airplane)

	light2Position = glm::vec3(-70.0f, 100.0f, 100.0f);
	light2Color = glm::vec3(0.5f, 1.0f, 1.0f);
	light2Direction = glm::vec3(0.0f, -1.0f, 0.0f);

	light2PositionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "light2Position");
	light2ColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "light2Color");
	light2DirectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "light2Direction");

	glUniform3fv(light2DirectionLoc, 1, glm::value_ptr(light2Direction));
	glUniform3fv(light2PositionLoc, 1, glm::value_ptr(light2Position));
	glUniform3fv(light2ColorLoc, 1, glm::value_ptr(light2Color));

	// Fog setup

	fogColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogColor");
	fogStartLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogStart");
	fogEndLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogEnd");

	glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));
	glUniform1f(fogStartLoc, fogStart);
	glUniform1f(fogEndLoc, fogEnd);

	// Light cube setup

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void drawObjects(gps::Shader shader, bool depthPass) {
	shader.useShaderProgram();

	// Render airport

	modelMatrix = airportModelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelMatrix));
		glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	airport.Draw(shader);

	airplaneOrbitAngle -= airplaneOrbitSpeed;
	if (airplaneOrbitAngle < 0.0f) {
		airplaneOrbitAngle += 360.0f;
	}

	// Render airplane w/o propeller

	glm::vec3 airplanePosition = orbitCenter + glm::vec3(
		orbitRadius * cos(glm::radians(airplaneOrbitAngle)),
		0.0f,
		orbitRadius * sin(glm::radians(airplaneOrbitAngle))
	);

	glm::vec3 toCenter = glm::normalize(orbitCenter - airplanePosition);
	float yaw = glm::degrees(atan2(toCenter.z, toCenter.x));

	modelMatrix = glm::translate(glm::mat4(1.0f), airplanePosition);
	modelMatrix = glm::rotate(modelMatrix, glm::radians(-yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));

	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelMatrix));
		glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	airplane.DrawExcept(shader, { propellerId });

	// Render propeller and airplane w/ propeller

	propellerRotationAngle += 45.0f;
	if (propellerRotationAngle >= 360.0f) {
		propellerRotationAngle -= 360.0f;
	}

	propellerModelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(propellerRotationAngle), glm::vec3(1.0f, 0.0f, 0.0f));

	modelMatrix = modelMatrix * propellerModelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelMatrix));
		glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	airplane.DrawPart(shader, { propellerId });

	// Render lamp

	modelMatrix = lamp1ModelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * modelMatrix));
		glUniformMatrix3fv(glGetUniformLocation(shader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

		glm::vec3 light2Position = airplanePosition + glm::vec3(0.0f, -25.0f, -5.0f);
		glUniform3fv(glGetUniformLocation(shader.shaderProgram, "light2Position"), 1, glm::value_ptr(light2Position));
	}
	lamp1.Draw(shader);
}

void renderSceneWithShadows() {
	glm::mat4 lightSpaceTrMatrix = computeLightSpaceTrMatrix();

	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(lightSpaceTrMatrix));

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	drawObjects(depthMapShader, true);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (showDepthMap) {
		glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
		glClear(GL_COLOR_BUFFER_BIT);
		screenQuadShader.useShaderProgram();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
	}
	else {
		glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		skyBox.Draw(skyboxShader, view, projection);
		myBasicShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		glUniform3fv(globalLightDirLoc, 1, glm::value_ptr(glm::normalize(globalLightDir)));

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

		glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(lightSpaceTrMatrix));

		drawObjects(myBasicShader, false);

		lightShader.useShaderProgram();
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		glm::mat4 model = glm::translate(glm::mat4(1.0f), globalLightDir);
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);
	}
}

void cleanup() {
	myWindow.Delete();
	//cleanup code for your own data
}

int main(int argc, const char* argv[]) {

	try {
		initOpenGLWindow();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	initOpenGLState();
	initFBO();
	initModels();
	initShaders();
	initUniforms();
	initSkyBox(false);
	setWindowCallbacks();
	glCheckError();

	globalLightDir.z += 1e-3;

	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
		currentTime = glfwGetTime();
		if (!canSwitchRenderMode && currentTime > debounceDelay) {
			canSwitchRenderMode = true;
		}
		renderSceneWithShadows();
		processMovement();
		present();
		updateLightFlicker(currentTime);

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

	return EXIT_SUCCESS;
}