#version 330 core
out vec4 FragColor;

uniform vec3 laserColor; // Passaremos vec3(1.0, 0.0, 0.0) para Vermelho ou vec3(0.0, 1.0, 0.0) para Verde

void main() {
    // Cor sólida e brilhante
    FragColor = vec4(laserColor, 1.0);
}