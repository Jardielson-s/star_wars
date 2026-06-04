#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h" // INSTANCIANDO NOSSO IMPORTER
#include <iostream>
#include <vector>
#include <ctime>

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

// Estrutura do Laser
struct Laser
{
  glm::vec3 Position;
  glm::vec3 Direction;
  float LifeTime;
};
std::vector<Laser> activeLasers;
float lastShotTime = 0.0f;
const float SHOT_COOLDOWN = 0.2f;

// Lasers do Inimigo
struct EnemyLaser
{
  glm::vec3 Position;
  glm::vec3 Direction;
  float LifeTime;
};
std::vector<EnemyLaser> activeEnemyLasers;
float lastEnemyShotTime = 0.0f;
const float ENEMY_SHOT_COOLDOWN = 1.8f; // Tempo em segundos entre os tiros da nave

// Estrutura de Partículas para a Explosão
struct Particle
{
  glm::vec3 Position;
  glm::vec3 Velocity;
  glm::vec4 Color;
  float LifeTime;
};
std::vector<Particle> activeParticles;

// Função auxiliar para gerar a explosão
void spawnExplosion(glm::vec3 position)
{
  int numberOfParticles = 40; // Quantidade de fragmentos na explosão
  for (int i = 0; i < numberOfParticles; i++)
  {
    Particle p;
    p.Position = position;

    // Gera direções e velocidades aleatórias nos eixos X, Y e Z
    float vx = ((rand() % 100) / 50.0f) - 1.0f;
    float vy = ((rand() % 100) / 50.0f) - 1.0f;
    float vz = ((rand() % 100) / 50.0f) - 1.0f;

    // Multiplica por uma velocidade de estilhaço (ex: entre 3.0 e 7.0)
    float speed = 3.0f + static_cast<float>(rand() % 4);
    p.Velocity = glm::normalize(glm::vec3(vx, vy, vz)) * speed;

    // Cor de fogo/plasma (Laranja/Amarelo que vai sumindo)
    p.Color = glm::vec4(1.0f, 0.4f + ((rand() % 6) / 10.0f), 0.0f, 1.0f);

    // Tempo de vida da partícula (ex: entre 0.5 e 1.2 segundos)
    p.LifeTime = 0.5f + static_cast<float>(rand() % 7) / 10.0f;

    activeParticles.push_back(p);
  }
}

// Gameplay e Alvo
struct Target
{
  glm::vec3 Position;
  float Radius;
  bool IsAlive;
  float RespawnTimer; // CONTADOR: Quanto tempo falta para renascer
  float RespawnDelay; // CONFIGURAÇÃO: Tempo padrão de espera (ex: 1.5 segundos)
};

// Teste aumentar ou diminuir esse raio até achar o "ponto doce" do tamanho da sua nave na tela
Target enemyTarget = {glm::vec3(0.0f, 0.0f, -8.0f), 1.6f, true, 0.0f, 1.5f};
unsigned int score = 0;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void processInput(GLFWwindow *window);

int main()
{
  srand(time(NULL));

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Star Wars Engine - Stage 7 (Assimp Loader)", NULL, NULL);
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

  // Inicializando Shaders
  Shader objectShader("shaders/vertex_shader.vs", "shaders/fragment_shader.fs");
  Shader skyboxShader("shaders/skybox.vs", "shaders/skybox.fs");
  Shader laserShader("shaders/laser.vs", "shaders/laser.fs");

  // CARREGANDO O MODELO 3D VIA ASSIMP
  Model enemyModel("assets/nave.obj");

  // Vértices do Skybox
  float skyboxVertices[] = {
      -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
      -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};

  float laserVertices[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.5f};

  // Inicialização dos buffers
  unsigned int skyboxVAO, skyboxVBO;
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);
  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

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

    // 1. MOTOR DE MOVIMENTAÇÃO DA NAVE INIMIGA
    if (enemyTarget.IsAlive)
    {
      float tempo = static_cast<float>(glfwGetTime());
      enemyTarget.Position.x = sin(tempo * 1.5f) * 3.0f;
      enemyTarget.Position.y = cos(tempo * 2.0f) * 1.8f;
    }
    else
    {
      enemyTarget.RespawnTimer -= deltaTime;
      if (enemyTarget.RespawnTimer <= 0.0f)
      {
        enemyTarget.IsAlive = true;
      }
    }

    // 2. INTELIGÊNCIA INIMIGA: ATACAR O JOGADOR
    if (enemyTarget.IsAlive)
    {
      float currentTime = static_cast<float>(glfwGetTime());
      if (currentTime - lastEnemyShotTime >= ENEMY_SHOT_COOLDOWN)
      {
        EnemyLaser el;
        el.Position = enemyTarget.Position;
        el.Direction = glm::normalize(camera.Position - enemyTarget.Position);
        el.LifeTime = 4.0f;

        activeEnemyLasers.push_back(el);
        lastEnemyShotTime = currentTime;
      }
    }

    // 3. ATUALIZAÇÃO DA FÍSICA DOS LASERS INIMIGOS
    for (auto it = activeEnemyLasers.begin(); it != activeEnemyLasers.end();)
    {
      it->Position += it->Direction * 20.0f * deltaTime;
      it->LifeTime -= deltaTime;

      float distToPlayer = glm::distance(it->Position, camera.Position);
      if (distToPlayer < 0.8f)
      {
        std::cout << "\n[ALERTA] VOCÊ FOI ATINGIDO PELO INIMIGO!" << std::endl;
        it = activeEnemyLasers.erase(it);
        continue;
      }

      if (it->LifeTime <= 0.0f)
      {
        it = activeEnemyLasers.erase(it);
      }
      else
      {
        ++it;
      }
    }

    // 4. MOTOR DE FÍSICA DOS LASERS DO JOGADOR (SUB-STEPPING)
    for (auto it = activeLasers.begin(); it != activeLasers.end();)
    {
      bool laserDestroyed = false;
      int subSteps = 3;
      glm::vec3 stepMove = (it->Direction * 35.0f * deltaTime) / (float)subSteps;

      for (int i = 0; i < subSteps; i++)
      {
        it->Position += stepMove;

        if (enemyTarget.IsAlive)
        {
          float dist = glm::distance(it->Position, enemyTarget.Position);

          if (dist < enemyTarget.Radius)
          {
            std::cout << "\n===================================" << std::endl;
            std::cout << "ALVO DESTRUÍDO!" << std::endl;
            score++;
            std::cout << "SCORE: " << score << std::endl;
            std::cout << "===================================\n"
                      << std::endl;

            spawnExplosion(enemyTarget.Position);
            activeEnemyLasers.clear(); // Limpa tiros remanescentes

            enemyTarget.IsAlive = false;
            enemyTarget.RespawnTimer = enemyTarget.RespawnDelay;

            laserDestroyed = true;
            break;
          }
        }
      }

      it->LifeTime -= deltaTime;

      if (it->LifeTime <= 0.0f || laserDestroyed)
      {
        it = activeLasers.erase(it);
      }
      else
      {
        ++it;
      }
    }

    // 5. MOTOR DE FÍSICA DAS PARTÍCULAS
    for (auto it = activeParticles.begin(); it != activeParticles.end();)
    {
      it->Position += it->Velocity * deltaTime;
      it->LifeTime -= deltaTime;
      it->Color.a = it->LifeTime;

      if (it->LifeTime <= 0.0f)
      {
        it = activeParticles.erase(it);
      }
      else
      {
        ++it;
      }
    }

    // --- PIPELINE DE RENDERIZAÇÃO ---
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

    // PASSO 1: DESENHAR O SKYBOX
    glDepthMask(GL_FALSE);
    skyboxShader.use();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(skyboxVAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);

    // PASSO 2: DESENHAR A NAVE INIMIGA
    if (enemyTarget.IsAlive)
    {
      objectShader.use();
      glm::vec3 luzPosicao(5.0f, 10.0f, 2.0f);

      glUniform3fv(glGetUniformLocation(objectShader.ID, "lightPos"), 1, glm::value_ptr(luzPosicao));
      glUniform3fv(glGetUniformLocation(objectShader.ID, "viewPos"), 1, glm::value_ptr(camera.Position));
      glUniform3f(glGetUniformLocation(objectShader.ID, "lightColor"), 1.0f, 1.0f, 1.0f);

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, enemyTarget.Position);
      model = glm::rotate(model, glm::radians(80.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      float escalaNave = 0.15f;
      model = glm::scale(model, glm::vec3(escalaNave, escalaNave, escalaNave));

      glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      enemyModel.Draw(objectShader);
    }

    // PASSO 3: DESENHAR AS PARTÍCULAS DA EXPLOSÃO
    laserShader.use();
    glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glPointSize(6.0f);
    glBindVertexArray(laserVAO);
    for (const auto &particle : activeParticles)
    {
      glm::mat4 pModel = glm::mat4(1.0f);
      pModel = glm::translate(pModel, particle.Position);
      glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(pModel));
      glUniform3f(glGetUniformLocation(laserShader.ID, "laserColor"), particle.Color.r, particle.Color.g, particle.Color.b);
      glDrawArrays(GL_POINTS, 0, 1);
    }
    glPointSize(1.0f);

    // PASSO 4: DESENHAR OS SEUS LASERS (VERDES)
    glUniform3f(glGetUniformLocation(laserShader.ID, "laserColor"), 0.0f, 1.0f, 0.0f); // Verde Puro
    glLineWidth(3.0f);
    for (const auto &laser : activeLasers)
    {
      glm::mat4 lModel = glm::inverse(glm::lookAt(laser.Position, laser.Position + laser.Direction, glm::vec3(0.0f, 1.0f, 0.0f)));
      lModel[3] = glm::vec4(laser.Position, 1.0f);
      glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lModel));
      glDrawArrays(GL_LINES, 0, 2);
    }

    // PASSO 5: DESENHAR OS LASERS INIMIGOS (VERMELHOS)

    // PASSO 5: DESENHAR O LASERS INIMIGOS (VERMELHOS)
    glUniform3f(glGetUniformLocation(laserShader.ID, "laserColor"), 1.0f, 0.0f, 0.0f); // Vermelho Puro
    glLineWidth(4.0f);                                                                 // Linha mais grossa para o perigo

    for (const auto &eLaser : activeEnemyLasers)
    {
      glm::mat4 elModel = glm::mat4(1.0f);

      // 1. Posiciona o início da linha na coordenada atual do laser inimigo
      elModel = glm::translate(elModel, eLaser.Position);

      // 2. Alinha a direção do laser baseado no vetor Direction que calculamos
      // Multiplicamos a escala para que a linha tenha um comprimento visível no espaço (ex: 1.0f)
      elModel = glm::scale(elModel, glm::vec3(1.0f, 1.0f, 1.0f));

      glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(elModel));

      // Desenha a linha vermelha usando o VAO genérico de laser
      glDrawArrays(GL_LINES, 0, 2);
    }
    glLineWidth(1.0f); // Reseta a largura da linha padrão

    // glUniform3f(glGetUniformLocation(laserShader.ID, "laserColor"), 1.0f, 0.0f, 0.0f); // Vermelho Puro
    // glLineWidth(4.0f);
    // for (const auto &eLaser : activeEnemyLasers)
    // {
    //   glm::mat4 elModel = glm::inverse(glm::lookAt(eLaser.Position, eLaser.Position + eLaser.Direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    //   elModel[3] = glm::vec4(eLaser.Position, 1.0f);
    //   glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(elModel));
    //   glDrawArrays(GL_LINES, 0, 2);
    // }
    // glLineWidth(1.0f); // Reseta a largura da linha

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

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

  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
  {
    float currentTime = static_cast<float>(glfwGetTime());
    if (currentTime - lastShotTime >= SHOT_COOLDOWN)
    {
      Laser newLaser;
      newLaser.Position = camera.Position + (camera.Front * 0.5f) - (camera.Up * 0.2f);
      newLaser.Direction = camera.Front;
      newLaser.LifeTime = 2.0f;
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