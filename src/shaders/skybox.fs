#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

// Gerador de ruído pseudo-aleatório 3D otimizado para alta frequência
float pseudoNoise(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

void main() {
    // Cor base do espaço profundo (quase preto, levemente azulado)
    vec3 spaceColor = vec3(0.005, 0.005, 0.015);
    
    // Multiplicamos por 300.0 para criar blocos minúsculos (do tamanho de pixels)
    vec3 st = TexCoords * 300.0;
    
    // Pega o ID único de cada "mini-bloco" espacial
    vec3 id = floor(st);
    
    // Gera um valor aleatório entre 0 e 1 para este bloco
    float starChance = pseudoNoise(id);
    
    // Se o valor for muito alto (raro), desenha uma estrela
    if (starChance > 0.994) {
        // Varia o brilho da estrela para dar sensação de distância
        float brightness = pseudoNoise(id + vec3(1.0));
        
        // Adiciona um tom levemente azulado ou avermelhado em algumas estrelas
        vec3 starColor = vec3(0.75, 0.85, 1.0) * brightness; 
        
        spaceColor += starColor;
    }
    
    FragColor = vec4(spaceColor, 1.0);
}