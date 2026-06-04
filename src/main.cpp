//  g++ main.cpp glad.c -o myapp -I. -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lassimp
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include <iostream>
#include <vector>

const unsigned int SCREEN_WIDTH = 1280;
const unsigned int SCREEN_HEIGHT = 720;

// Configuração da Câmera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCREEN_WIDTH / 2.0f;
float lastY = SCREEN_HEIGHT / 2.0f;
bool firstMouse = true;

// Variáveis de Tempo
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- NOVO: Estrutura do Laser ---
struct Laser
{
  glm::vec3 Position;
  glm::vec3 Direction;
  float LifeTime; // Tempo de vida do tiro para sumir e não pesar na memória
};
std::vector<Laser> activeLasers;
float lastShotTime = 0.0f;
const float SHOT_COOLDOWN = 0.2f; // Intervalo mínimo entre tiros (0.2 segundos)

// --- NOVO: Estrutura do Alvo e Gameplay ---
struct Target
{
  glm::vec3 Position;
  float Radius; // Raio da nossa esfera de colisão
  bool IsAlive;
};

Target enemyTarget = {glm::vec3(0.0f, 0.0f, -6.0f), 0.75f, true}; // Posição inicial, raio de 0.75 e vivo
unsigned int score = 0;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void processInput(GLFWwindow *window);

int main()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Star Wars Engine - Stage 6 (Lasers Active)", NULL, NULL);
  if (window == NULL)
  {
    std::cerr << "Falha ao criar a janela GLFW" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cerr << "Falha ao inicializar o GLAD" << std::endl;
    return -1;
  }

  glEnable(GL_DEPTH_TEST);

  // Inicializando nossos 3 Shaders
  Shader objectShader("shaders/vertex_shader.vs", "shaders/fragment_shader.fs");
  Shader skyboxShader("shaders/skybox.vs", "shaders/skybox.fs");
  Shader laserShader("shaders/laser.vs", "shaders/laser.fs");

  // Vértices do Cubo Alvo (Nave Inimiga fictícia)
  float cubeVertices[] = {
      -0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f, 0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f,
      0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f,
      -0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f, 0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.8f, 0.0f, 0.0f,
      0.5f, 0.5f, 0.5f, 0.8f, 0.0f, 0.0f, -0.5f, 0.5f, 0.5f, 0.8f, 0.0f, 0.0f, -0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f,
      -0.5f, 0.5f, 0.5f, 0.8f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f,
      -0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f, -0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f, -0.5f, 0.5f, 0.5f, 0.8f, 0.0f, 0.0f,
      0.5f, 0.5f, 0.5f, 0.8f, 0.0f, 0.0f, 0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f,
      0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f, 0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.8f, 0.0f, 0.0f,
      -0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f, 0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f,
      0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f, -0.5f, -0.5f, 0.5f, 0.8f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.8f, 0.0f, 0.0f,
      -0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f, 0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.8f, 0.0f, 0.0f,
      0.5f, 0.5f, 0.5f, 0.8f, 0.0f, 0.0f, -0.5f, 0.5f, 0.5f, 0.8f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.8f, 0.0f, 0.0f};

  // Vértices do Skybox
  float skyboxVertices[] = {
      -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
      -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};

  // --- NOVO: Vértices de uma linha esticada que representará o feixe laser ---
  float laserVertices[] = {
      0.0f, 0.0f, 0.0f, // Início da linha (na ponta da arma)
      0.0f, 0.0f, -1.5f // Fim da linha (esticada para frente)
  };

  // Inicialização dos buffers normais
  unsigned int cubeVAO, cubeVBO;
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &cubeVBO);
  glBindVertexArray(cubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  unsigned int skyboxVAO, skyboxVBO;
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);
  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // --- NOVO: Inicialização do Buffer do Laser ---
  unsigned int laserVAO, laserVBO;
  glGenVertexArrays(1, &laserVAO);
  glGenBuffers(1, &laserVBO);
  glBindVertexArray(laserVAO);
  glBindBuffer(GL_ARRAY_BUFFER, laserVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(laserVertices), laserVertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // --- GAME LOOP ---
  while (!glfwWindowShouldClose(window))
  {
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window);

    // --- ATUALIZAR FÍSICA DOS LASERS ---
    // --- MOTOR DE FÍSICA E GAMEPLAY ATUALIZADO ---
    for (auto it = activeLasers.begin(); it != activeLasers.end();)
    {
      // 1. Mover o laser para frente
      it->Position += it->Direction * 30.0f * deltaTime; // Aumentei a velocidade para 30.0f
      it->LifeTime -= deltaTime;

      bool laserDestroyed = false;

      // 2. Checar colisão se o inimigo estiver vivo
      if (enemyTarget.IsAlive)
      {
        // Calcula a distância 3D entre o laser atual e o alvo
        float dist = glm::distance(it->Position, enemyTarget.Position);

        // Se a distância for menor que o raio do inimigo (considerando o raio do laser quase 0)
        if (dist < enemyTarget.Radius)
        {
          std::cout << "ALVO ATINGIDO! BELO TIRO, PILOTO! " << std::endl;
          score++;
          std::cout << "SCORE ATUAL: " << score << "\n--------------------" << std::endl;

          // "Destrói" o inimigo temporariamente e teleporta ele para uma nova posição aleatória
          float randomX = ((rand() % 100) / 10.0f) - 5.0f; // Entre -5 e 5
          float randomY = ((rand() % 60) / 10.0f) - 3.0f;  // Entre -3 e 3
          float randomZ = -((rand() % 50) / 10.0f) - 4.0f; // Entre -4 e -9 (sempre na frente)

          enemyTarget.Position = glm::vec3(randomX, randomY, randomZ);

          laserDestroyed = true; // Marca que o laser explodiu no impacto
        }
      }

      // 3. Gerenciamento de memória do laser
      if (it->LifeTime <= 0.0f || laserDestroyed)
      {
        it = activeLasers.erase(it); // Remove o laser
      }
      else
      {
        ++it;
      }
    }
    // for (auto it = activeLasers.begin(); it != activeLasers.end();)
    // {
    //   it->Position += it->Direction * 25.0f * deltaTime; // Velocidade do tiro (25 unidades por segundo)
    //   it->LifeTime -= deltaTime;

    //   if (it->LifeTime <= 0.0f)
    //   {
    //     it = activeLasers.erase(it); // Remove o laser expirado da lista
    //   }
    //   else
    //   {
    //     ++it;
    //   }
    // }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

    // 1. DESENHAR O SKYBOX
    glDepthMask(GL_FALSE);
    skyboxShader.use();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(skyboxVAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);

    // 2. DESENHAR O CUBO ALVO (MUDAMOS A COR PARA VERMELHO NEON)
    // objectShader.use();
    // glm::mat4 model = glm::mat4(1.0f);
    // model = glm::translate(model, glm::vec3(0.0f, 0.0f, -5.0f)); // Um pouco mais distante para dar alvo
    // model = glm::rotate(model, (float)glfwGetTime() * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
    // glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    // glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    // glBindVertexArray(cubeVAO);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // glDrawArrays(GL_TRIANGLES, 0, 36);
    // 2. DESENHAR O CUBO ALVO (Apenas se estiver ativo)
    if (enemyTarget.IsAlive)
    {
      objectShader.use();
      glm::mat4 model = glm::mat4(1.0f);

      // Usa a posição dinâmica do alvo calculada pelo motor de colisão
      model = glm::translate(model, enemyTarget.Position);
      model = glm::rotate(model, (float)glfwGetTime() * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

      glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

      glBindVertexArray(cubeVAO);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // 3. --- NOVO: RENDERIZAR OS LASERS ATIVOS ---
    laserShader.use();
    glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Define a cor do laser (Verde Rebelde / X-Wing)
    glUniform3f(glGetUniformLocation(laserShader.ID, "laserColor"), 0.0f, 1.0f, 0.3f);

    // engrossa a linha do OpenGL para o laser ficar visível e imponente
    glLineWidth(4.0f);
    glBindVertexArray(laserVAO);

    // for (const auto &laser : activeLasers)
    // {
    //   glm::mat4 lModel = glm::mat4(1.0f);
    //   lModel = glm::translate(lModel, laser.Position);

    //   // Alinha a rotação do feixe laser com a direção em que ele foi disparado
    //   // Como nossa linha aponta para -Z, apontamos o modelo para a direção salva
    //   lModel = glm::matrixInterface(lModel, glm::lookAt(laser.Position, laser.Position + laser.Direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    //   lModel = glm::inverse(lModel); // Inverte porque lookAt gera a inversa da visualização

    //   glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lModel));
    //   glDrawArrays(GL_LINES, 0, 2);
    // }
    for (const auto &laser : activeLasers)
    {
      // 1. Matriz de translação (Leva o laser até a posição dele no espaço)
      glm::mat4 lModel = glm::mat4(1.0f);

      // 2. Cria a matriz de rotação para alinhar o feixe com a direção do tiro
      // Usamos o inverso do lookAt para transformar orientação de câmera em orientação de objeto
      glm::mat4 rotation = glm::inverse(glm::lookAt(laser.Position, laser.Position + laser.Direction, glm::vec3(0.0f, 1.0f, 0.0f)));

      // 3. Combina translação e rotação
      lModel = rotation;
      // Sobrescreve a posição da matriz de rotação com a posição real do laser
      lModel[3] = glm::vec4(laser.Position, 1.0f);

      glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lModel));
      glDrawArrays(GL_LINES, 0, 2);
    }
    glLineWidth(1.0f); // Reseta a largura da linha

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &cubeVAO);
  glDeleteBuffers(1, &cubeVBO);
  glDeleteVertexArrays(1, &skyboxVAO);
  glDeleteBuffers(1, &skyboxVBO);
  glDeleteVertexArrays(1, &laserVAO);
  glDeleteBuffers(1, &laserVBO);
  glfwTerminate();
  return 0;
}

void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, deltaTime);

  // --- NOVO: LÓGICA DE DISPARO (SPACEBAR) ---
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
  {
    float currentTime = static_cast<float>(glfwGetTime());
    if (currentTime - lastShotTime >= SHOT_COOLDOWN)
    {
      Laser newLaser;
      // Spawna o laser um pouco abaixo da câmera para simular que saiu do bico/asa da nave
      newLaser.Position = camera.Position + (camera.Front * 0.5f) - (camera.Up * 0.2f);
      newLaser.Direction = camera.Front;
      newLaser.LifeTime = 2.0f; // some após 2 segundos no espaço

      activeLasers.push_back(newLaser);
      lastShotTime = currentTime;
    }
  }
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);
  if (firstMouse)
  {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }
  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos;
  lastX = xpos;
  lastY = ypos;
  camera.ProcessMouseMovement(xoffset, yoffset);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  glViewport(0, 0, width, height);
}