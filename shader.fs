#version 330 core
out vec4 FragColor;
in vec3 ourColor;
in float heightY;

uniform vec3 color;
uniform bool use_line;
uniform bool use_gridline;
uniform bool use_heatmap;
uniform float min_height;
uniform float max_height;

vec3 computeColor(float value)
{
    if (value <= 0.2) {
        return mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), value / 0.2);
    } else if (value <= 0.4) {
        return mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), (value - 0.2) / 0.2);
    } else if (value <= 0.6) {
        return mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (value - 0.4) / 0.2);
    } else if (value <= 0.8) {
        return mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.5, 0.0), (value - 0.6) / 0.2);
    } else {
        return mix(vec3(1.0, 0.5, 0.0), vec3(1.0, 0.0, 0.0), (value - 0.8) / 0.2);
    }
}

void main()
{
    if (use_line) {
        FragColor = vec4(color, 0.1);
    }
    else if (use_gridline) {
        FragColor = vec4(color, 1.0);
    }
    else if (use_heatmap) {
        float normalizedHeight = (heightY - min_height) / (max_height - min_height);
        normalizedHeight = clamp(normalizedHeight, 0.0, 1.0);
        vec3 heatmap_color = computeColor(normalizedHeight);

        FragColor = vec4(heatmap_color, 1.0);
    }
    else {
        FragColor = vec4(ourColor, 1.0);
    }
}
