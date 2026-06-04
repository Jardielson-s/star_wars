#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

uniform sampler2D texture_diffuse1; 

void main()
{
    // Recupera a cor da textura
    vec4 texel = texture(texture_diffuse1, TexCoords);
    vec3 textureColor = texel.rgb;
    
    // --- DIAGNÓSTICO DE SEGURANÇA ---
    // Se a textura falhar, não tiver coordenadas UV válidas ou retornar preto/transparente,
    // nós forçamos uma cor visível (Magenta/Rosa) para a nave reaparecer e sabermos onde ela está.
    if (length(textureColor) < 0.05 || texel.a < 0.1) {
        textureColor = vec3(1.0, 0.0, 1.0); 
    }
    
    // 1. AMBIENTE
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
  	
    // 2. DIFUSA
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 3. ESPECULAR
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 result = (ambient + diffuse + specular) * textureColor;
    FragColor = vec4(result, 1.0);
}