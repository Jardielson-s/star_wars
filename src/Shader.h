#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
  unsigned int ID; // ID do programa do Shader

  // Construtor lê e reconstrói o shader
  Shader(const char *vertexPath, const char *fragmentPath)
  {
    // 1. Recuperar o código-fonte do vertex/fragment shader do filepath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // Garante que os objetos ifstream podem lançar exceções:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
      // Abrir arquivos
      vShaderFile.open(vertexPath);
      fShaderFile.open(fragmentPath);
      std::stringstream vShaderStream, fShaderStream;
      // Ler o conteúdo dos arquivos para os streams
      vShaderStream << vShaderFile.rdbuf();
      fShaderStream << fShaderFile.rdbuf();
      // Fechar arquivos
      vShaderFile.close();
      fShaderFile.close();
      // Converter stream em string
      vertexCode = vShaderStream.str();
      fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure &e)
    {
      std::cerr << "ERRO::SHADER::ARQUIVO_NAO_FOI_LIDO_COM_SUCESSO" << std::endl;
    }
    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();

    // 2. Compilar os Shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // Programa do Shader
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    // Deletar os shaders, pois eles já foram linkados no programa e não são mais necessários
    glDeleteShader(vertex);
    glDeleteShader(fragment);
  }

  // Ativar o shader
  void use()
  {
    glUseProgram(ID);
  }

  // Funções utilitárias para configurar Uniforms (usaremos muito no futuro)
  void setBool(const std::string &name, bool value) const
  {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
  }
  void setInt(const std::string &name, int value) const
  {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
  }
  void setFloat(const std::string &name, float value) const
  {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
  }

private:
  // Utilitário para checar erros de compilação/linkagem
  void checkCompileErrors(unsigned int shader, std::string type)
  {
    int success;
    char infoLog[512];
    if (type != "PROGRAM")
    {
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERRO::SHADER_COMPILATION_ERROR de tipo: " << type << "\n"
                  << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
      }
    }
    else
    {
      glGetProgramiv(shader, GL_LINK_STATUS, &success);
      if (!success)
      {
        glGetProgramInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERRO::PROGRAM_LINKING_ERROR de tipo: " << type << "\n"
                  << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
      }
    }
  }
};
#endif