#version 330 core
out vec4 FragColor;
in vec3 ourColor;

uniform vec3 color;
uniform bool use_line;

void main()
{
    if(use_line) {
        FragColor = vec4(color, 1.0);
    } else {
        FragColor = vec4(ourColor, 1.0);
    }
    
}
