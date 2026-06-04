#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

class Model
{
public:
  // Armazena todas as malhas que compõem o modelo
  std::vector<Mesh> meshes;
  std::string directory;

  // Construtor, espera o caminho completo do arquivo 3D
  Model(std::string const &path)
  {
    loadModel(path);
  }

  // Desenha o modelo inteiro percorrendo cada uma de suas malhas
  void Draw(Shader &shader)
  {
    for (unsigned int i = 0; i < meshes.size(); i++)
      meshes[i].Draw(shader);
  }

private:
  // Carrega o modelo usando Assimp e popula o vetor 'meshes'
  void loadModel(std::string const &path)
  {
    Assimp::Importer importer;
    // Lemos o arquivo aplicando flags para triangularizar faces complexas e inverter o eixo Y das texturas se necessário
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    // Checa se houve erros no carregamento
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
      std::cerr << "ERRO::ASSIMP:: " << importer.GetErrorString() << std::endl;
      return;
    }
    // Recupera o diretório do arquivo para buscar as texturas na mesma pasta mais tarde
    directory = path.substr(0, path.find_last_of('/'));

    // Processa o nó raiz recursivamente
    processNode(scene->mRootNode, scene);
  }

  // Processa os nós da hierarquia da Assimp de forma recursiva
  void processNode(aiNode *node, const aiScene *scene)
  {
    // Processa todas as malhas do nó atual (se houver)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      meshes.push_back(processMesh(mesh, scene));
    }
    // Depois faz o mesmo para cada um dos nós filhos
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
      processNode(node->mChildren[i], scene);
    }
  }

  // Converte uma malha nativa da Assimp (aiMesh) para a nossa classe Mesh customizada
  Mesh processMesh(aiMesh *mesh, const aiScene *scene)
  {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // 1. Processa os Vértices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
      Vertex vertex;
      glm::vec3 vector;

      // Posições
      vector.x = mesh->mVertices[i].x;
      vector.y = mesh->mVertices[i].y;
      vector.z = mesh->mVertices[i].z;
      vertex.Position = vector;

      // Normais
      if (mesh->HasNormals())
      {
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;
      }

      // Coordenadas de Textura (UV)
      if (mesh->mTextureCoords[0])
      {
        glm::vec2 vec;
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
        vertex.TexCoords = vec;
      }
      else
      {
        vertex.TexCoords = glm::vec2(0.0f, 0.0f);
      }

      vertices.push_back(vertex);
    }

    // 2. Processa os Índices (Faces do objeto)
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
      aiFace face = mesh->mFaces[i];
      for (unsigned int j = 0; j < face.mNumIndices; j++)
        indices.push_back(face.mIndices[j]);
    }

    // Retorna a nossa malha montada (Por enquanto sem texturas externas ativas)
    return Mesh(vertices, indices, textures);
  }
};
#endif