#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoords = aPos;
    // Remove a translação da matriz de visualização para o skybox não sair do lugar
    mat4 staticView = mat4(mat3(view)); 
    gl_Position = projection * staticView * vec4(aPos, 1.0);
}