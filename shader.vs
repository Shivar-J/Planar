#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 ourColor;
out float heightY;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    heightY = worldPos.y;
    gl_Position = projection * view * worldPos;
    ourColor = aColor;
}
