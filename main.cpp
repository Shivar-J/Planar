#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include "shader.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "camera.hpp"
#include "exprtk.hpp"

float minX = -25.0, maxX = 25.0, minY = minX, maxY = maxX;
int samplesX = 1000, samplesY = samplesX;

std::string equation;

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 6.0f, 12.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool mouseFocusGLFW = false;
bool showControls = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLuint VAO, VBO;
std::vector<glm::vec3> points_vec;

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (mouseFocusGLFW) {
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::ROLL_LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::ROLL_RIGHT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::UP, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			camera.ProcessKeyboard(Camera_Movement::DOWN, deltaTime);
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
		mouseFocusGLFW = !mouseFocusGLFW;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	if (mouseFocusGLFW) {
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
	
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void generate_vertices(float minX, float maxX, float minY, float maxY, int samplesX, int samplesY, std::string expression) {
	exprtk::symbol_table<float> symbol_table;
	exprtk::expression<float> expr;
	exprtk::parser<float> parser;

	float stepX = (maxX - minX) / samplesX;
	float stepY = (maxY - minY) / samplesY;

	const double e = 2.71828182845904523536028747135266249775724709369996;
	const double pi = 3.14159265358979323846264338327950288419716939937510;

	float x, y;

	symbol_table.add_constant("e", e);

	symbol_table.add_pi();
	
	symbol_table.add_variable("x", x);
	symbol_table.add_variable("y", y);

	expr.register_symbol_table(symbol_table);
	parser.compile(expression, expr);

	for (float x = minX; x <= maxX; x += stepX) {
		for (float y = minY; y <= maxY; y += stepY) {
			symbol_table.get_variable("x")->ref() = x;
			symbol_table.get_variable("y")->ref() = y;
			float z = expr.value();
			points_vec.push_back(glm::vec3(x, z, y));
		}
	}
}

void rerender(std::string expression) {
	equation = expression;
	points_vec.clear();
	
	generate_vertices(minX, maxX, minY, maxY, samplesX, samplesY, expression);

	glm::vec3* new_points = points_vec.data();

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, points_vec.size() * sizeof(glm::vec3), new_points, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

int main() {
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Planar", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	Shader shader("shader.vs", "shader.fs");

	shader.use();

	glm::mat4 model = glm::mat4(1.0f);
	shader.setMat4("model", model);

	float grid[] = {
		-1000.0f,  0.0f, 0.0f,
		 1000.0f,  0.0f, 0.0f,

		0.0f, -1000.0f, 0.0f,
		0.0f,  1000.0f, 0.0f,

		0.0f, 0.0f, -1000.0f,
		0.0f, 0.0f,  1000.0f
	};

	GLuint VAO_lines, VBO_lines;
	glGenVertexArrays(1, &VAO_lines);
	glGenBuffers(1, &VBO_lines);
	glBindVertexArray(VAO_lines);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
	glBufferData(GL_ARRAY_BUFFER, sizeof(grid), grid, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	float data[] = {1.0, 0.5, 0.2};
	char buf[256] = "";

	while (!glfwWindowShouldClose(window)) {
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		
		if (!mouseFocusGLFW) {
			io.WantCaptureMouse = true;
			io.WantCaptureKeyboard = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else {
			io.WantCaptureMouse = false;
			io.WantCaptureKeyboard = false;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}

		shader.use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		shader.setMat4("projection", projection);
		glm::mat4 view = camera.GetViewMatrix();
		shader.setMat4("view", view);
		glm::vec3 colour = glm::make_vec3(data);
		shader.setVec3("color", colour);
		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, points_vec.size());

		shader.setVec3("color", glm::vec3(1.0, 1.0, 1.0));
		glBindVertexArray(VAO_lines);
		glDrawArrays(GL_LINES, 0, sizeof(grid) / sizeof(float) / 3);
		
		ImGui::Begin("Planar");

		if (showControls) {
			ImGui::OpenPopup("Controls");
		}
		if (ImGui::BeginPopupModal("Controls", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("WASD to move");
			ImGui::Text("Left Control: Down\nSpace: Up");
			ImGui::Text("Q: rotate left\nE: rotate right");
			ImGui::Text("C: toggle input");
			ImGui::Separator();
			if (ImGui::Button("Close")) {
				showControls = !showControls;
				mouseFocusGLFW = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::Checkbox("Toggle Input", &mouseFocusGLFW);
		ImGui::InputText("Equation", buf, 256);
		ImGui::ColorEdit4("Colour", data);
		ImGui::SliderInt("Sample Size", &samplesX, 1, 10000);
		ImGui::SliderFloat("Minimum X", &minX, -100, 0);
		ImGui::SliderFloat("Maxmimum X", &maxX, 0, 100);
		ImGui::SliderFloat("Minimum Y", &minY, -100, 0);
		ImGui::SliderFloat("Maxmimum Y", &maxY, 0, 100);
		ImGui::Text("Camera Position: %s", glm::to_string(camera.Position).c_str());
		if (ImGui::Button("Render")) {
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);

			rerender(equation);
		}
		ImGui::SameLine();
		if (ImGui::Button("Render New Equation")) {
			equation = buf;

			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);

			rerender(equation);
		}

		ImGui::Text("%.1f FPS", io.Framerate);

		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO_lines);
	glDeleteBuffers(1, &VBO_lines);
	glfwTerminate();
	return 0;
}