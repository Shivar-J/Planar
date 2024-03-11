#include "equation.hpp"
#include "shader.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "camera.hpp"
#include "exprtk.hpp"

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
std::vector<Equation> equations;

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
	if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS)
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
// float minX, float maxX, float minY, float maxY, int samplesX, int samplesY, std::string expression
void generate_vertices(Equation& equation) {
	exprtk::symbol_table<float> symbol_table;
	exprtk::expression<float> expr;
	exprtk::parser<float> parser;

	float stepX = (equation.max_x - equation.min_x) / equation.sample_size;
	float stepY = (equation.max_y - equation.min_y) / equation.sample_size;

	const double e = 2.71828182845904523536028747135266249775724709369996;
	const double pi = 3.14159265358979323846264338327950288419716939937510;

	float x, y;

	symbol_table.add_constant("e", e);

	symbol_table.add_pi();
	
	symbol_table.add_variable("x", x);
	symbol_table.add_variable("y", y);

	expr.register_symbol_table(symbol_table);
	parser.compile(equation.buf, expr);

	symbol_table.get_variable("x")->ref() = x;
	symbol_table.get_variable("y")->ref() = y;

	if (equation.is_3d) {
		for (x = equation.min_x; x <= equation.max_x; x += stepX) {
			for (y = equation.min_y; y <= equation.max_y; y += stepY) {
				equation.points_vec_equation.push_back(glm::vec3(x, expr.value(), y));
				equation.points_vec_equation.push_back(glm::make_vec3(equation.data));
			}
		}
	}
	else if(equation.is_3d == false) {
		for (x = equation.min_x; x <= equation.max_x; x += stepX) {
			for (y = equation.min_y; y <= equation.max_y; y += stepY) {
				equation.points_vec_equation.push_back(glm::vec3(x, expr.value(), 0));
				equation.points_vec_equation.push_back(glm::make_vec3(equation.data));
			}
		}
	}
}

void rerender(Shader& shader) {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	points_vec.clear();

	for (auto& equation : equations) {
		points_vec.insert(points_vec.begin(), equation.points_vec_equation.begin(), equation.points_vec_equation.end());
	}
	glm::vec3* new_points = points_vec.data();

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, points_vec.size() * sizeof(glm::vec3), new_points, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
}

void remove_equation(int index) {
	equations.erase(equations.begin() + index);
}

void draw_equation_input(Equation& equation, Shader& shader, size_t index) {
	ImGui::InputText("Equation", equation.buf, sizeof(equation.buf));
	ImGui::ColorEdit3("Colour", equation.data);
	ImGui::SliderInt("Sample Size", &equation.sample_size, 1, 10000);
	ImGui::SliderFloat("Minimum X", &equation.min_x, -100, -0);
	ImGui::SliderFloat("Maximum X", &equation.max_x, 1, 100);
	ImGui::SliderFloat("Minimum Y", &equation.min_y, -100, -0);
	ImGui::SliderFloat("Maximum Y", &equation.max_y, 1, 100);
	ImGui::Checkbox("Toggle 3D", &equation.is_3d);
	if (ImGui::Button("Remove Equation")) {
		equation.points_vec_equation.clear();

		remove_equation(index);

		generate_vertices(equation);
		rerender(shader);
	}
	ImGui::SameLine();
	if (ImGui::Button("Render")) {
		equation.points_vec_equation.clear();

		generate_vertices(equation);
		rerender(shader);
	}
}

void draw_equations(Shader& shader) {
	for (size_t i = 0; i < equations.size(); i++) {
		ImGui::PushID(static_cast<int>(i));
		draw_equation_input(equations[i], shader, i);
		
		ImGui::PopID();
		ImGui::Separator();
	}
}

void add_equation() {
	Equation new_equation;

	equations.push_back(new_equation);
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
	Equation first_equation;
	equations.push_back(first_equation);

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

		shader.setBool("use_line", true);
		shader.setVec3("color", glm::vec3(1.0, 1.0, 1.0));
		glBindVertexArray(VAO_lines);
		glDrawArrays(GL_LINES, 0, sizeof(grid) / sizeof(float) / 3);
		shader.setBool("use_line", false);
		
		ImGui::Begin("Planar");

		if (showControls) {
			ImGui::OpenPopup("Controls");
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5, 0.5));
		}
		if (ImGui::BeginPopupModal("Controls", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::SetItemDefaultFocus();
			ImGui::Text("WASD to move");
			ImGui::Text("Left Control: Down\nSpace: Up");
			ImGui::Text("Q: rotate left\nE: rotate right");
			ImGui::Text("`: toggle input");
			ImGui::Text("Escape: close program");
			ImGui::Separator();
			if (ImGui::Button("Close")) {
				showControls = !showControls;
				mouseFocusGLFW = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::Text("Vertex density might negatively effect performance. Adjust sample size accordingly.");
			ImGui::Text("Be sure to change the colour when working with multiple equations.");

			ImGui::EndPopup();
		}

		draw_equations(shader);

		if (equations.size() >= 1) {
			for (auto& equation : equations) {
				glm::vec3 colour = glm::make_vec3(equation.data);
				shader.setVec3("color", colour);
			}
		}

		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, points_vec.size());

		if (ImGui::Button("Add Equation")) {
			add_equation();
		}
		ImGui::Text("Camera Position: %s", glm::to_string(camera.Position).c_str());
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