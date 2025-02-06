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
	float min_z = -25.0;
	float max_z = 25.0;
	std::vector<glm::vec3> points_vec_equation;
	bool is_3d = true;
	bool is_visible = true;
	float opacity = 1.0f;
	bool is_mesh = false;
	std::vector<unsigned int> indices;
	float discontinuity_threshold = 10.0f;
};

struct Point {
	char x_buf[256] = "";
	char y_buf[256] = "";
	char z_buf[256] = "";
	float point_buf[3] = { 0.0, 0.0, 0.0 };
	std::vector<glm::vec3> point_data;
	float data[3] = { 1.0, 0.5, 0.2 };
};