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
  // Substitua pelo caminho do seu arquivo .obj
  Model enemyModel("assets/nave.obj");

  // Vértices fixos do Skybox (Apenas posições, o resto é gerado pelo shader)
  float skyboxVertices[] = {
      -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
      -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};

  float laserVertices[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.5f};

  // Inicialização dos buffers do Skybox e Laser
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

    // --- MOTOR DE MOVIMENTAÇÃO E GAMEPLAY COM DELAY ---
    if (enemyTarget.IsAlive)
    {
      // A nave só se move se estiver viva
      float tempo = static_cast<float>(glfwGetTime());
      enemyTarget.Position.x = sin(tempo * 1.5f) * 3.0f;
      enemyTarget.Position.y = cos(tempo * 2.0f) * 1.8f;
    }
    else
    {
      // Se estiver morta, fazemos a contagem regressiva usando o deltaTime
      enemyTarget.RespawnTimer -= deltaTime;

      // Quando o tempo acabar, a nave "renasce" cheia de vida
      if (enemyTarget.RespawnTimer <= 0.0f)
      {
        enemyTarget.IsAlive = true;
        std::cout << "NOVA NAVE INIMIGA DETECTADA! PREPARE-SE!" << std::endl;
      }
    }

    // --- MOTOR DE FÍSICA COM SUB-STEPPING ---
    for (auto it = activeLasers.begin(); it != activeLasers.end();)
    {
      bool laserDestroyed = false;
      int subSteps = 3;
      glm::vec3 stepMove = (it->Direction * 35.0f * deltaTime) / (float)subSteps;

      for (int i = 0; i < subSteps; i++)
      {
        it->Position += stepMove;

        // IMPORTANTE: Só checa colisão se a nave estiver viva no frame
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

            // --- ENCOSTA O GATILHO DA EXPLOSÃO AQUI ---
            spawnExplosion(enemyTarget.Position);

            // --- ATIVANDO O DELAY DE MORTE ---
            enemyTarget.IsAlive = false;
            enemyTarget.RespawnTimer = enemyTarget.RespawnDelay; // Começa a contagem de 1.5s

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

    // --- MOTOR DE FÍSICA DAS PARTÍCULAS ---
    for (auto it = activeParticles.begin(); it != activeParticles.end();)
    {
      it->Position += it->Velocity * deltaTime;
      it->LifeTime -= deltaTime;

      // Efeito opcional: faz a partícula perder o brilho (alpha) sumindo gradualmente
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

    // 2. DESENHAR O MODELO IMPORTADO (Nave Inimiga)
    if (enemyTarget.IsAlive)
    {
      objectShader.use();
      glm::mat4 model = glm::mat4(1.0f);

      // PASSO 1: Posiciona a nave na coordenada física do jogo
      model = glm::translate(model, enemyTarget.Position);

      // PASSO 2: ROTAÇÃO DE ALINHAMENTO FIXO
      // Removemos o glfwGetTime(). Agora a nave não gira mais feito pião.
      // Se com 180 graus ela ainda não estiver de frente para você,
      // mude esse valor para 0.0f, 90.0f ou -90.0f até ela cravar os olhos na sua câmera.
      model = glm::rotate(model, glm::radians(80.0f), glm::vec3(0.0f, 1.0f, 0.0f));

      // PASSO 3: ESCALA LOCAL
      float escalaNave = 0.15f;
      model = glm::scale(model, glm::vec3(escalaNave, escalaNave, escalaNave));

      // PASSO 4: CORREÇÃO MANUAL DE PIVÔ LOCAL
      model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));

      glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      enemyModel.Draw(objectShader);
    }

    // 3. RENDERIZAR OS LASERS ATIVOS
    laserShader.use();
    glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(laserShader.ID, "laserColor"), 0.0f, 1.0f, 0.3f);
    glLineWidth(4.0f);
    glBindVertexArray(laserVAO);

    // 4. RENDERIZAR AS PARTÍCULAS DA EXPLOSÃO
    laserShader.use();
    glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Configura o OpenGL para desenhar pontos maiores na tela
    glPointSize(6.0f);
    glBindVertexArray(laserVAO); // Usa o VAO genérico de posições

    for (const auto &particle : activeParticles)
    {
      glm::mat4 pModel = glm::mat4(1.0f);
      pModel = glm::translate(pModel, particle.Position);
      glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(pModel));

      // Passa a cor dinâmica de cada partícula para o shader (Laranja/Amarelo)
      glUniform3f(glGetUniformLocation(laserShader.ID, "laserColor"), particle.Color.r, particle.Color.g, particle.Color.b);

      // Desenha apenas o primeiro vértice do buffer como um ponto isolado no espaço
      glDrawArrays(GL_POINTS, 0, 1);
    }
    glPointSize(1.0f); // Reseta o tamanho do ponto padrão

    for (const auto &laser : activeLasers)
    {
      glm::mat4 lModel = glm::inverse(glm::lookAt(laser.Position, laser.Position + laser.Direction, glm::vec3(0.0f, 1.0f, 0.0f)));
      lModel[3] = glm::vec4(laser.Position, 1.0f);
      glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lModel));
      glDrawArrays(GL_LINES, 0, 2);
    }
    glLineWidth(1.0f);

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