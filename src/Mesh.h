#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

// Estrutura que define os dados completos de um único vértice de um modelo 3D complexo
struct Vertex
{
  glm::vec3 Position;  // Coordenadas X, Y, Z
  glm::vec3 Normal;    // Vetor Normal (vital para a Etapa de Iluminação de Phong)
  glm::vec2 TexCoords; // Coordenadas U, V para mapear imagens/texturas na lataria
};

struct Texture
{
  unsigned int id;
  std::string type; // Ex: "texture_diffuse" (cor) ou "texture_specular" (brilho)
  std::string path; // Guardamos o caminho do arquivo para evitar recarregar imagens repetidas
};

#include "Shader.h" // Garante acesso à nossa classe de Shader

class Mesh
{
public:
  // Dados da Malha
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;
  unsigned int VAO;

  // Construtor
  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
  {
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    // Agora que temos os dados, configuramos os buffers no OpenGL
    setupMesh();
  }

  // Renderiza a malha
  void Draw(Shader &shader)
  {
    // Vincula as texturas correspondentes (usaremos mais na etapa de texturização)
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    for (unsigned int i = 0; i < textures.size(); i++)
    {
      glActiveTexture(GL_TEXTURE0 + i); // Ativa a unidade de textura correta antes de vincular

      std::string name = textures[i].type;
      if (name == "texture_diffuse")
        shader.setInt((name + std::to_string(diffuseNr++)).c_str(), i);
      else if (name == "texture_specular")
        shader.setInt((name + std::to_string(specularNr++)).c_str(), i);

      glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // Desenha a malha usando Element Buffers (Índices)
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Sempre resete para o padrão após desenhar
    glActiveTexture(GL_TEXTURE0);
  }

private:
  // Buffers internos
  unsigned int VBO, EBO;

  // Inicializa todos os objetos de buffer e layouts de atributos
  void setupMesh()
  {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Carrega os dados no Vertex Buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    // Carrega os dados no Element Buffer (Índices)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Configuração dos Atributos de Vértice (Layouts do Vertex Shader)
    // 1. Posições (X, Y, Z) -> Location = 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

    // 2. Normais (X, Y, Z) -> Location = 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal));

    // 3. Coordenadas de Textura (U, V) -> Location = 2
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
  }
};

#endif