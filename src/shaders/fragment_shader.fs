#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;   // Posição do "Sol" sideral
uniform vec3 viewPos;    // Posição da câmera do jogador
uniform vec3 lightColor; // Cor da luz (ex: branca ou azulada)
uniform vec3 objectColor;// Cor base da nave

void main()
{
    // 1. AMBIENTE
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
  	
    // 2. DIFUSA
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 3. ESPECULAR (Brilho Metálico)
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // 32 é o brilho do material
    vec3 specular = specularStrength * spec * lightColor;  
        
    // Resultado Final combinando as 3 forças
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}