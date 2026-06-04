#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 projection2D; // Matriz Ortográfica

void main()
{
    // Forçamos o Z a ser 0.0 para ficar sempre colado na frente do ecrã
    gl_Position = projection2D * vec4(aPos, 0.0, 1.0);
}