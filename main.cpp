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

// window
gps::Window myWindow;

// matrices
glm::mat4 airportModelMatrix, airplaneModelMatrix;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 airportNormalMatrix, airplaneNormalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint airportModelLoc, airplaneModelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint airportNormalMatrixLoc, airplaneNormalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

// camera
gps::Camera myCamera(
	glm::vec3(0.0f, 25.0f, -15.0f),
	glm::vec3(0.0f, 20.0f, 15.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.75f;
GLfloat airplaneOrbitAngle = 0.0f; // Airplane's current angle along the orbit
glm::vec3 orbitCenter = glm::vec3(0.0f, 50.0f, 0.0f); // Center of the circular path
float orbitRadius = 100.0f; // Radius of the circular path


GLboolean pressedKeys[1024];

// models
gps::Model3D airport;
gps::Model3D airplane;
GLfloat angle;

// shaders
gps::Shader myBasicShader, skyboxShader;

// skybox
gps::SkyBox skyBox;

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
	// Update the OpenGL viewport to match the new window dimensions
	if (width == 0 || height == 0) {
		return;
	}
	glViewport(0, 0, width, height);

	// Recalculate the projection matrix based on the new aspect ratio
	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

	// Update the projection matrix uniform in the shader
	myBasicShader.useShaderProgram();
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS) {
			pressedKeys[key] = true;
		}
		else if (action == GLFW_RELEASE) {
			pressedKeys[key] = false;
		}
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	//TODO
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
		//update view matrix
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		// compute normal matrix for teapot
		airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	}

	if (pressedKeys[GLFW_KEY_UP]) {
		myCamera.rotate(cameraSpeed, 0.0f);
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	}

	if (pressedKeys[GLFW_KEY_DOWN]) {
		myCamera.rotate(-cameraSpeed, 0.0f);
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	}

	if (pressedKeys[GLFW_KEY_LEFT]) {
		myCamera.rotate(0.0f, cameraSpeed);
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	}

	if (pressedKeys[GLFW_KEY_RIGHT]) {
		myCamera.rotate(0.0f, -cameraSpeed);
		view = myCamera.getViewMatrix();
		myBasicShader.useShaderProgram();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	}

}

void initOpenGLWindow() {
	myWindow.Create(1024, 768, "OpenGL Project Core");
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
}

void initShaders() {
	myBasicShader.loadShader(
		"shaders/basic.vert",
		"shaders/basic.frag");
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
}

void initSkyBox() {
	std::vector<const GLchar*> faces;
	faces.push_back("skybox/posx.png"); // right
	faces.push_back("skybox/negx.png"); // left
	faces.push_back("skybox/posy.png"); // top
	faces.push_back("skybox/negy.png"); // bottom
	faces.push_back("skybox/negz.png"); // back
	faces.push_back("skybox/posz.png"); // front
	skyBox.Load(faces);
}

void initUniforms() {
	myBasicShader.useShaderProgram();

	airportModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
	//model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	airportModelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	airplaneModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, 75.0f, 70.0f));
	airplaneModelMatrix = glm::rotate(airplaneModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	airplaneModelMatrix = glm::rotate(airplaneModelMatrix, glm::radians(-13.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	airplaneModelMatrix = glm::scale(airplaneModelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
	airplaneModelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	// compute normal matrix for teapot
	airportNormalMatrix = glm::mat3(glm::inverseTranspose(view * airportModelMatrix));
	airportNormalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	airplaneNormalMatrix = glm::mat3(glm::inverseTranspose(view * airplaneModelMatrix));
	airplaneNormalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");


	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
		(float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
		0.1f, 2000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(0.5f, 0.5f, 0.5f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void renderAirport(gps::Shader shader) {
	// select active shader program
	shader.useShaderProgram();

	//send teapot model matrix data to shader
	glUniformMatrix4fv(airportModelLoc, 1, GL_FALSE, glm::value_ptr(airportModelMatrix));

	//send teapot normal matrix data to shader
	glUniformMatrix3fv(airportNormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(airportNormalMatrix));

	// draw airport
	airport.Draw(shader);
}

void renderAirplane(gps::Shader shader) {
	shader.useShaderProgram();

	airplaneOrbitAngle -= 0.5f;
	if (airplaneOrbitAngle < 0.0f) {
		airplaneOrbitAngle += 360.0f;
	}

	glm::vec3 airplanePosition = orbitCenter + glm::vec3(
		orbitRadius * cos(glm::radians(airplaneOrbitAngle)), 
		0.0f,                                               
		orbitRadius * sin(glm::radians(airplaneOrbitAngle))
	);

	glm::vec3 toCenter = glm::normalize(orbitCenter - airplanePosition);

	float yaw = glm::degrees(atan2(toCenter.z, toCenter.x));

	airplaneModelMatrix = glm::translate(glm::mat4(1.0f), airplanePosition);
	airplaneModelMatrix = glm::rotate(airplaneModelMatrix, glm::radians(-yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	airplaneModelMatrix = glm::rotate(airplaneModelMatrix, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));    
	airplaneModelMatrix = glm::scale(airplaneModelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));

	glUniformMatrix4fv(airplaneModelLoc, 1, GL_FALSE, glm::value_ptr(airplaneModelMatrix));

	glUniformMatrix3fv(airplaneNormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(airplaneNormalMatrix));

	airplane.Draw(shader);
}

void renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//render the scene
	skyBox.Draw(skyboxShader, view, projection);
	// render the airport
	renderAirport(myBasicShader);
	renderAirplane(myBasicShader);

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
	initModels();
	initShaders();
	initUniforms();
	initSkyBox();
	setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
		processMovement();
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

	return EXIT_SUCCESS;
}