#version 330 core
out vec4 FragColor;

uniform vec3 hudColor;

void main()
{
    FragColor = vec4(hudColor, 1.0);
}