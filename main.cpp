#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "equation.hpp"
#include "shader.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "camera.hpp"
#include "exprtk.hpp"
#include "updater.hpp"
#include <cstring>
#include "stb_image.h"

unsigned int SCR_WIDTH = 1280;
unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 6.0f, 12.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool mouseFocusGLFW = false;
bool showControls = true;
bool use_heatmap = false;
bool show_gridlines = true;
bool show_lines = true;
float min_height = FLT_MAX;
float max_height = -FLT_MAX;
float min_x_val = -100;
float max_x_val = 100;
float min_y_val = -100;
float max_y_val = 100;
float max_view_distance = 250.0f;
float point_size = 1.0f;
int max_depth = 6;
double derivative_threshold = 5.0;

char import_filepath[256] = "";
char export_filepath[256] = "";

float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLuint VAO, VBO, EBO;
std::vector<glm::vec3> points_vec;
std::vector<unsigned int> indices_vec;
std::vector<Equation> equations;
std::vector<Point> points;

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
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
		use_heatmap = !use_heatmap;
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

void generate_vertices(Equation& equation) {
	exprtk::symbol_table<float> symbol_table;
	exprtk::expression<float> expr;
	exprtk::parser<float> parser;

	float spacing_tolerance = 0.001f;
	float stepX = (equation.max_x - equation.min_x) / equation.sample_size;
	float stepY = (equation.max_y - equation.min_y) / equation.sample_size;

	const double e = 2.71828182845904523536028747135266249775724709369996;
	const double pi = 3.14159265358979323846264338327950288419716939937510;

	float x, y, z;

	symbol_table.add_constant("e", e);

	symbol_table.add_pi();

	symbol_table.add_variable("x", x);
	symbol_table.add_variable("y", y);
	symbol_table.add_variable("z", z);

	expr.register_symbol_table(symbol_table);
	parser.compile(equation.buf, expr);

	symbol_table.get_variable("x")->ref() = x;
	symbol_table.get_variable("y")->ref() = y;
	symbol_table.get_variable("z")->ref() = z;

	min_height = FLT_MAX;
	max_height = -FLT_MAX;

	double epsilon = 1e-6;

	auto adaptive_samples = [&](auto&& func, float min, float max) {
		std::vector<float> samples;
		std::function<void(float, float, float, float, int)> subdivide = [&](float x0, float x1, float y0, float y1, int depth) {
			if (depth >= max_depth) {
				samples.push_back(x0);
				return;
			}

			const float x_mid = (x0 + x1) * 0.5f;
			const float y_mid = func(x_mid);

			const float dy_left = std::abs((y_mid - y0) / (x_mid - x0 + epsilon));
			const float dy_right = std::abs((y1 - y_mid) / (x1 - x_mid + epsilon));

			if (dy_left > derivative_threshold || dy_right > derivative_threshold) {
				subdivide(x0, x_mid, y0, y_mid, depth + 1);
				subdivide(x_mid, x1, y_mid, y1, depth + 1);
			}
			else {
				samples.push_back(x0);
			}
			};
		std::vector<float> base_x;
		for (int i = 0; i < equation.sample_size; i++) {
			base_x.push_back(min + (max - min) * (i / float(equation.sample_size - 1)));
		}

		for (size_t i = 0; i < base_x.size() - 1; i++) {
			const float x0 = base_x[i];
			const float x1 = base_x[i + 1];
			subdivide(x0, x1, func(x0), func(x1), 0);
		}

		samples.push_back(max);
		std::sort(samples.begin(), samples.end());
		return samples;
	};

	auto safe_eval = [&](float x_val, float y_val = 0) {
		try {
			x = x_val;
			y = equation.is_3d ? y_val : 0;
			return expr.value();
		}
		catch (...) {
			return NAN;
		}
	};

	if (equation.is_3d) {
		auto x_samples = adaptive_samples([&](float x) { return safe_eval(x, equation.min_y); }, equation.min_x, equation.max_x);
		auto y_samples = adaptive_samples([&](float y) { return safe_eval(equation.min_x, y); }, equation.min_y, equation.max_y);

		for (float x : x_samples) {
			for (float y : y_samples) {
				const float z = safe_eval(x, y);
				if (!std::isnan(z)) {
					equation.points_vec_equation.emplace_back(x, z, y);
					equation.points_vec_equation.emplace_back(glm::make_vec3(equation.data));
					min_height = std::min(min_height, z);
					max_height = std::max(max_height, z);
				}
			}
		}

		if (equation.is_mesh) {
			const int cols = x_samples.size();
			const int rows = y_samples.size();

			for (int y = 0; y < rows - 1; y++) {
				for (int x = 0; x < cols - 1; x++) {
					const int i0 = y * cols + x;
					const int i1 = y * cols + (x + 1);
					const int i2 = (y + 1) * cols + x;
					const int i3 = (y + 1) * cols + (x + 1);

					if (std::isnan(equation.points_vec_equation[i0].y) ||
						std::isnan(equation.points_vec_equation[i1].y) ||
						std::isnan(equation.points_vec_equation[i2].y) ||
						std::isnan(equation.points_vec_equation[i3].y))
						continue;
					equation.indices.insert(equation.indices.end(), {
						static_cast<unsigned int>(i0),
						static_cast<unsigned int>(i1),
						static_cast<unsigned int>(i2),
						static_cast<unsigned int>(i1),
						static_cast<unsigned int>(i3),
						static_cast<unsigned int>(i2),
						});
				}
			}
		}
	}
	else {
		auto x_samples = adaptive_samples([&](float x) { return safe_eval(x, equation.min_y); }, equation.min_x, equation.max_x);

		for (float x : x_samples) {
			const float y = safe_eval(x);
			if (!std::isnan(y)) {
				equation.points_vec_equation.emplace_back(x, y, 0);
				equation.points_vec_equation.emplace_back(glm::make_vec3(equation.data));
				min_height = std::min(min_height, y);
				max_height = std::max(max_height, y);
			}
		}
	}
}

void rerender(Shader& shader) {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	points_vec.clear();
	indices_vec.clear();

	size_t vertex_offset = 0;
	for (auto& equation : equations) {
		if (equation.is_visible) {
			points_vec.insert(points_vec.begin(), equation.points_vec_equation.begin(), equation.points_vec_equation.end());
		}

		if (equation.is_mesh) {
			for (auto& index : equation.indices) {
				indices_vec.push_back(index + vertex_offset);
			}
		}
		vertex_offset += equation.points_vec_equation.size();
	}

	for (auto& point : points) {
		points_vec.insert(points_vec.end(), point.point_data.begin(), point.point_data.end());
	}

	glm::vec3* new_points = points_vec.data();

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, points_vec.size() * sizeof(glm::vec3), new_points, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_vec.size() * sizeof(unsigned int), indices_vec.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
}

void remove_equation(int index) {
	equations.erase(equations.begin() + index);
}

void remove_point(int index) {
	points.erase(points.begin() + index);
}

void draw_equation_input(Equation& equation, Shader& shader, size_t index) {
	ImGui::InputText("Equation", equation.buf, sizeof(equation.buf));
	ImGui::ColorEdit3("Colour", equation.data);
	ImGui::SliderInt("Sample Size", &equation.sample_size, 1, 10000);
	ImGui::SliderFloat("Minimum X", &equation.min_x, min_x_val, -0);
	ImGui::SliderFloat("Maximum X", &equation.max_x, 1, max_x_val);
	ImGui::SliderFloat("Minimum Y", &equation.min_y, min_y_val, -0);
	ImGui::SliderFloat("Maximum Y", &equation.max_y, 1, max_y_val);
	ImGui::SliderFloat("Opacity", &equation.opacity, 0, 1);
	bool visibility_toggle = ImGui::Checkbox("Toggle Visibility", &equation.is_visible);
	bool toggle_3d = ImGui::Checkbox("Toggle 3D", &equation.is_3d);
	bool heatmap_toggle = ImGui::Checkbox("Toggle Heatmap", &use_heatmap);
	bool mesh_toggle = ImGui::Checkbox("Toggle Mesh (might not work for all functions)", &equation.is_mesh);
	if (ImGui::Button("Remove Equation")) {
		equation.points_vec_equation.clear();
		equation.indices.clear();

		remove_equation(index);

		generate_vertices(equation);
		rerender(shader);
	}
	ImGui::SameLine();
	if (ImGui::Button("Render")) {
		equation.points_vec_equation.clear();
		equation.indices.clear();

		generate_vertices(equation);
		rerender(shader);
	}
	if (visibility_toggle || toggle_3d || heatmap_toggle || mesh_toggle) {
		equation.points_vec_equation.clear();
		equation.indices.clear();

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

void draw_points_input(Point& point, Shader& shader, size_t index) {
	ImGui::InputFloat3("Point", point.point_buf);
	ImGui::ColorEdit3("Colour", point.data);
	
	if (ImGui::Button("Remove Point")) {
		point.point_data.clear();

		remove_point(index);

		rerender(shader);
	}
	ImGui::SameLine();
	if (ImGui::Button("Render")) {
		point.point_data.clear();

		point.point_data.push_back(glm::make_vec3(point.point_buf));
		point.point_data.push_back(glm::make_vec3(point.data));

		rerender(shader);
	}
}

void draw_points(Shader& shader) {
	for (size_t i = 0; i < points.size(); i++) {
		ImGui::PushID(static_cast<int>(i));

		draw_points_input(points[i], shader, i);

		ImGui::PopID();
		ImGui::Separator();
	}
}

void add_equation() {
	Equation new_equation;

	equations.push_back(new_equation);
}

void add_point() {
	Point new_point;

	points.push_back(new_point);
}

void run_updater(const std::string& updater_exe, const std::string& exe_path) {
	std::string command = updater_exe + " " + exe_path;
	std::system(command.c_str());
}

std::string remove_quotes(const std::string& str) {
	std::string result;
	result.reserve(str.size());

	for (char ch : str) {
		if (ch != '"' && ch != '\'')
			result += ch;
	}

	return result;
}

void import_data(const std::string& filename) {
	std::ifstream infile(filename);

	if (!infile.is_open()) {
		return;
	}

	std::string line;
	
	while (std::getline(infile, line)) {
		std::istringstream iss(line);
		std::string type;
		iss >> type;

		if (type == "Equation") {
			Equation eq;
			iss >> eq.data[0] >> eq.data[1] >> eq.data[2];
			iss >> eq.sample_size;
			iss >> eq.min_x >> eq.max_x >> eq.min_y >> eq.max_y;
			iss >> eq.is_visible >> eq.is_3d;

			std::string buf;
			std::getline(iss, buf, '\n');
			buf = remove_quotes(buf);
			std::strncpy(eq.buf, buf.c_str(), sizeof(eq.buf) - 1);
			eq.buf[sizeof(eq.buf) - 1] = '\0';
			
			equations.push_back(eq);
		}
		else if (type == "Point") {
			Point pt;
			iss >> pt.point_buf[0] >> pt.point_buf[1] >> pt.point_buf[2];
			iss >> pt.data[0] >> pt.data[1] >> pt.data[2];
			points.push_back(pt);
		}
	}

	infile.close();
}

void export_data(const std::string& filename) {
	std::string filepath = filename + ".mat";
	std::ofstream outfile(filepath);

	if (!outfile.is_open()) {
		return;
	}

	for (const auto& eq : equations) {
		outfile << "Equation \""
			<< eq.data[0] << " " << eq.data[1] << " " << eq.data[2] << " "
			<< eq.sample_size << " "
			<< eq.min_x << " " << eq.max_x << " "
			<< eq.min_y << " " << eq.max_y << " "
			<< eq.is_visible << " " << eq.is_3d
			<< eq.buf << "\" " << "\n";
	}

	for (const auto& pt : points) {
		outfile << "Point " << pt.point_buf[0] << " " << pt.point_buf[1] << " " << pt.point_buf[2] << " "
			<< pt.data[0] << " " << pt.data[1] << " " << pt.data[2] << "\n";
	}

	outfile.close();
}

int main() {
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	Updater updater("Shivar-J", "planar");
	bool update_needed = updater.check_for_updates();

	if (update_needed) {
		run_updater("autoupdate.exe", "Planar.exe");
	}

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

	GLFWimage images[1];
	images[0].pixels = stbi_load("planaricon.png", &images[0].width, &images[0].height, 0, 4);
	glfwSetWindowIcon(window, 1, images);
	stbi_image_free(images[0].pixels);

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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	Shader shader("shader.vs", "shader.fs");

	shader.use();

	glm::mat4 model = glm::mat4(1.0f);
	shader.setMat4("model", model);

	const float grid_size = 1000.0f;
	const float grid_spacing = 1.0f;

	int num_lines = static_cast<int>(grid_size / grid_spacing) * 2 + 1;
	int num_vertices = num_lines * 12;

	std::vector<float> grid_vertices;
	grid_vertices.reserve(num_vertices * 3);

	for (float i = -grid_size; i <= grid_size; i += grid_spacing) {
		grid_vertices.push_back(-grid_size);
		grid_vertices.push_back(0.0f);
		grid_vertices.push_back(i);
		grid_vertices.push_back(grid_size);
		grid_vertices.push_back(0.0f);
		grid_vertices.push_back(i);
	}

	for (float i = -grid_size; i <= grid_size; i += grid_spacing) {
		grid_vertices.push_back(i);
		grid_vertices.push_back(0.0f);
		grid_vertices.push_back(-grid_size);
		grid_vertices.push_back(i);
		grid_vertices.push_back(0.0f);
		grid_vertices.push_back(grid_size);
	}

	for (float i = -grid_size; i <= grid_size; i += grid_spacing) {
		grid_vertices.push_back(0.0f);
		grid_vertices.push_back(-grid_size);
		grid_vertices.push_back(i);
		grid_vertices.push_back(0.0f);
		grid_vertices.push_back(grid_size);
		grid_vertices.push_back(i);
	}

	for (float i = -grid_size; i <= grid_size; i += grid_spacing) {
		grid_vertices.push_back(0.0f);
		grid_vertices.push_back(i);
		grid_vertices.push_back(grid_size);
		grid_vertices.push_back(0.0f);
		grid_vertices.push_back(i);
		grid_vertices.push_back(-grid_size);
	}

	for (float i = -grid_size; i <= grid_size; i += grid_spacing) {
		grid_vertices.push_back(grid_size);
		grid_vertices.push_back(i);
		grid_vertices.push_back(0.0f);
		grid_vertices.push_back(-grid_size);
		grid_vertices.push_back(i);
		grid_vertices.push_back(0.0f);
	}

	for (float i = -grid_size; i <= grid_size; i += grid_spacing) {
		grid_vertices.push_back(i);
		grid_vertices.push_back(grid_size);
		grid_vertices.push_back(0.0f);
		grid_vertices.push_back(i);
		grid_vertices.push_back(-grid_size);
		grid_vertices.push_back(0.0f);
	}

	GLuint VAO_grid, VBO_grid;
	glGenVertexArrays(1, &VAO_grid);
	glGenBuffers(1, &VBO_grid);

	glBindVertexArray(VAO_grid);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_grid);
	glBufferData(GL_ARRAY_BUFFER, grid_vertices.size() * sizeof(float), grid_vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	float gridlines[] = {
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(gridlines), gridlines, GL_STATIC_DRAW);
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

	float data[] = { 1.0, 0.5, 0.2 };
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
		glDepthMask(GL_FALSE);
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, max_view_distance);
		shader.setMat4("projection", projection);
		glm::mat4 view = camera.GetViewMatrix();
		shader.setMat4("view", view);
		glm::vec3 colour = glm::make_vec3(data);
		shader.setVec3("color", colour);

		shader.setFloat("min_height", min_height);
		shader.setFloat("max_height", max_height);
		shader.setFloat("point_size", point_size);
		shader.setFloat("point_opacity", 1.0f);

		if (show_gridlines) {
			shader.setBool("use_gridline", true);
			shader.setVec3("color", glm::vec3(1.0, 1.0, 1.0));
			glBindVertexArray(VAO_lines);
			glDrawArrays(GL_LINES, 0, sizeof(gridlines) / sizeof(float) / 3);
			glBindVertexArray(0);
			shader.setBool("use_gridline", false);
		}

		if (show_lines) {
			shader.setBool("use_line", true);
			shader.setVec3("color", glm::vec3(1.0, 1.0, 1.0));
			glBindVertexArray(VAO_grid);
			glDrawArrays(GL_LINES, 0, num_vertices);
			glBindVertexArray(0);
			shader.setBool("use_line", false);
		}

		glDepthMask(GL_TRUE);
		
		shader.setBool("use_heatmap", use_heatmap);

		ImGui::Begin("Planar");

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Options")) {
				ImGui::InputFloat("Change Max View Distance", &max_view_distance);
				ImGui::Separator();
				ImGui::InputDouble("Adjust Derivative Threshold", &derivative_threshold);
				ImGui::InputInt("Adjust Depth", &max_depth);
				ImGui::Separator();
				ImGui::Checkbox("Show Axes", &show_gridlines);
				ImGui::Checkbox("Show Grid Lines", &show_lines);
				ImGui::InputFloat("Change Minimum X value", &min_x_val);
				ImGui::InputFloat("Change Maximum X value", &max_x_val);
				ImGui::InputFloat("Change Minimum Y value", &min_y_val);
				ImGui::InputFloat("Change Maximum Y value", &max_y_val);
				ImGui::InputFloat("Set Point Size", &point_size);
				ImGui::Separator();
				ImGui::InputText("Filepath for Import (don't forget .mat extension)", import_filepath, sizeof(import_filepath));
				if (ImGui::Button("Import Equations")) {
					import_data(import_filepath);
				}
				ImGui::InputText("Filename for Export (no extension required)", export_filepath, sizeof(export_filepath));
				if (ImGui::Button("Export Equations")) {
					export_data(export_filepath);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

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
			ImGui::Text("`: toggle keyboard and mouse input");
			ImGui::Text("H: toggle heatmap");
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
		draw_points(shader);


		if (equations.size() >= 1) {
			for (auto& equation : equations) {
				glm::vec3 colour = glm::make_vec3(equation.data);
				shader.setVec3("color", colour);
				shader.setFloat("point_opacity", equation.opacity);
			}
		}

		glBindVertexArray(VAO);

		if (!indices_vec.empty())
			glDrawElements(GL_TRIANGLES, indices_vec.size(), GL_UNSIGNED_INT, 0);

		size_t array_start = indices_vec.empty() ? 0 : points_vec.size() - indices_vec.size();
		glDrawArrays(GL_POINTS, array_start, points_vec.size() - array_start);

		if (ImGui::Button("Add Equation")) {
			add_equation();
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Point")) {	
			add_point();
		}
		ImGui::Text("Camera Position: %s", glm::to_string(camera.Position).c_str());
		ImGui::Text("%.1f FPS", io.Framerate);
		ImGui::Text("Min Height: %.2f", min_height);
		ImGui::Text("Max Height: %.2f", max_height);
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
	glDeleteVertexArrays(1, &VAO_grid);
	glDeleteBuffers(1, &VBO_grid);
	glfwTerminate();
	return 0;
}