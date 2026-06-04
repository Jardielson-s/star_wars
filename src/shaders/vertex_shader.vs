#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

// Matrizes de transformação 3D
uniform mat4 model;      // Posição/Rotação/Escala do objeto no mundo
uniform mat4 view;       // Posição/Orientação da câmera
uniform mat4 projection; // Lente da câmera (Perspectiva 3D)

void main() {
    // A multiplicação em GLSL ocorre da direita para a esquerda
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    ourColor = aColor;
}