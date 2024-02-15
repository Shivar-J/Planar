#include "shader.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "camera.hpp"
#include "equation.hpp"
#include <algorithm>
#include <execution>
#include <functional>
#include <omp.h>
#include <chrono>
#include <iomanip>
#include <gl/GL.h>

class Vertex {
public:
	double x, y, z;
	Vertex(double x, double y, double z) : x(x), y(y), z(z) {}
};

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

std::vector<Vertex> generateVertices(double minX, double maxX, double minY, double maxY, int samplesX, int samplesY, std::string expression) {
	Equation function;
	std::vector<Vertex> vertices;

	double stepX = (maxX - minX) / samplesX;
	double stepY = (maxY - minY) / samplesY;

	int num_threads = omp_get_max_threads();
	omp_set_num_threads(num_threads);

#pragma omp parallel for collapse(2)
	for (double x = minX; x <= maxX; x += stepX) {
		for (double y = minY; y <= maxY; y += stepY) {
			double z = function.evaluateWithXY(expression, x, y);
#pragma omp critical
			vertices.push_back(Vertex(x, y, z));
		}
	}
	return vertices;
}

int main() {
	std::ofstream file;
	file.open("vertices.txt");
	double minX = -10.0, maxX = 10.0, minY = -10.0, maxY = 10.0;
	int samplesX = 100, samplesY = 100;

	std::cout << "Enter Equation" << std::endl;
	std::string equation;
	std::getline(std::cin, equation);
	equation.erase(remove(equation.begin(), equation.end(), ' '), equation.end());
	file << equation << std::endl;

	auto start = std::chrono::high_resolution_clock::now();

	std::vector<Vertex> vertices = generateVertices(minX, maxX, minY, maxY, samplesX, samplesY, equation);
	std::vector<glm::vec3> points_vec;

	auto end = std::chrono::high_resolution_clock::now();

	auto start_write = std::chrono::high_resolution_clock::now();

	std::vector <std::string> output;
	output.resize(vertices.size());

#pragma omp parallel for
	for (int i = 0; i < vertices.size(); i++) {
		const Vertex& v = vertices[i];
		std::ostringstream ss;
		ss << "x: " << v.x << ", y: " << v.y << ", z: " << v.z << std::endl;
		points_vec.push_back(glm::vec3(v.x, v.y, v.z));
		output[i] = ss.str();
	}

	for (const auto& s : output) {
		file << s;
	}

	auto end_write = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> vertex_runtime = end - start;
	std::chrono::duration<double> write_runtime = end - start;

	std::cout << "Generate vertices runtime: " << std::fixed << vertex_runtime.count() << std::setprecision(5) << "s" << std::endl;
	std::cout << "Print vertices runtime: " << std::fixed << write_runtime.count() << std::setprecision(5) << "s" << std::endl;

	glm::vec3* points = points_vec.data();

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//equation handling here

	GLFWwindow* window = glfwCreateWindow(800, 600, "3d Visualizer", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, points_vec.size() * sizeof(glm::vec3), points, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);


	Shader shader("shader.vs", "shader.fs");

	shader.use();

	glm::mat4 model = glm::mat4(1.0f);
	shader.setMat4("model", model);

	while (!glfwWindowShouldClose(window)) {
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		shader.setMat4("projection", projection);

		glm::mat4 view = camera.GetViewMatrix();
		shader.setMat4("view", view);

		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, points_vec.size());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glfwTerminate();
	return 0;
}