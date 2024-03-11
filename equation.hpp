#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include <vector>
#include <string>

#include <glm/glm.hpp>

struct Equation {
	char buf[256] = "";
	float data[3] = { 1.0, 0.5, 0.2 };
	int sample_size = 1000;
	float min_x = -25.0;
	float max_x = 25.0;
	float min_y = -25.0;
	float max_y = 25.0;
	std::vector<glm::vec3> points_vec_equation;
	bool is_3d = true;
};

struct Mesh {
	std::vector<glm::vec3> vertices;
	std::vector<unsigned int> indices;
};