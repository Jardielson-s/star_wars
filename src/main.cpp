#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h" // INSTANCIANDO NOSSO IMPORTER
#include <iostream>
#include <vector>
#include <ctime>
#include "soloud.h"
#include "soloud_wav.h"

SoLoud::Soloud soloud;
SoLoud::Wav soundLaser;
SoLoud::Wav soundExplosion;
SoLoud::Wav soundEnemyLaser;
SoLoud::Wav soundGodzillaBlast;

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
  bool IsSpecial = false;
  int Damage;
  int Scale;
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

// --- SISTEMA DE ONDAS E MÚLTIPLOS INIMIGOS ---
struct Enemy
{
  glm::vec3 Position;
  float Radius = 1.2f;
  bool IsAlive = true;
  float PhaseOffset; // Deslocamento de tempo para movimentos variados
  float SpeedMultiplier;
};

std::vector<Enemy> activeEnemies;

int currentWave = 1;
bool waveCleared = false;
float waveTransitionTimer = 0.0f;
const float WAVE_DELAY = 2.0f; // Tempo de espera entre as ondas

int killCount = 0;
int killCountToSpecial = 0;
bool canUseSpecial = false;
bool isChargingSpecial = false;
float chargeTimer = 0.0f;
const float CHARGE_TIME = 26.0f;

// Estrutura de Partículas para a Explosão
struct Particle
{
  glm::vec3 Position;
  glm::vec3 Velocity;
  glm::vec4 Color;
  float LifeTime;
};
std::vector<Particle> activeParticles;

// --- ESTADO DO JOGADOR E DO JOGO ---
struct Player
{
  int HP = 100;
  int MaxHP = 100;
  bool IsAlive = true;
};
Player player;

enum GameState
{
  PLAYING,
  GAME_OVER
};
GameState currentGameState = PLAYING;
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
void spawnWave(int waveNumber);
void initAudio();
void dispararLaserEspecial();
void spawnParticle(glm::vec3 position, glm::vec3 velocity);
void stopGodzillaSound(unsigned int godzillaHandle);

int main()
{
  srand(time(NULL));

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // soloud.init();
  initAudio();
  // Carrega os arquivos (certifique-se de que existem na pasta)
  // soundLaser.load("assets/laser.wav");
  // soundExplosion.load("audio/tokyo_drift.wav");

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
  // Inicialização do Shader do HUD
  Shader hudShader("shaders/hud.vs", "shaders/hud.fs");

  // Vértices de um quadrado/retângulo 2D na origem (0,0) até (1,1)
  float hudVertices[] = {
      0.0f, 1.0f, // Canto superior esquerdo
      0.0f, 0.0f, // Canto inferior esquerdo
      1.0f, 0.0f, // Canto inferior direito

      0.0f, 1.0f, // Canto superior esquerdo
      1.0f, 0.0f, // Canto inferior direito
      1.0f, 1.0f  // Canto superior direito
  };

  unsigned int hudVAO, hudVBO;
  glGenVertexArrays(1, &hudVAO);
  glGenBuffers(1, &hudVBO);
  glBindVertexArray(hudVAO);
  glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(hudVertices), hudVertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glGenVertexArrays(1, &laserVAO);
  glGenBuffers(1, &laserVBO);
  glBindVertexArray(laserVAO);
  glBindBuffer(GL_ARRAY_BUFFER, laserVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(laserVertices), laserVertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Inicializa a primeira onda de inimigos antes do loop começar
  spawnWave(currentWave);
  unsigned int godzillaHandle = 0;
  // --- GAME LOOP ---
  while (!glfwWindowShouldClose(window))
  {
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window);

    // Só processa o comportamento dos inimigos se o jogo estiver rodando
    if (currentGameState == PLAYING)
    {
      bool anyEnemyAlive = false;

      // Loop para mover e fazer cada nave viva atirar
      for (auto &enemy : activeEnemies)
      {
        if (!enemy.IsAlive)
          continue;

        anyEnemyAlive = true; // Achou pelo menos uma nave viva

        // 1. MOTOR DE MOVIMENTAÇÃO INDIVIDUAL (Usando PhaseOffset)
        float tempo = static_cast<float>(glfwGetTime()) * enemy.SpeedMultiplier;
        enemy.Position.x = sin(tempo * 1.5f + enemy.PhaseOffset) * 4.0f;
        enemy.Position.y = cos(tempo * 2.0f + enemy.PhaseOffset) * 1.8f;

        // 2. INTELIGÊNCIA INIMIGA: CADA NAVE ATIRA INDEPENDENTEMENTE
        float currentTime = static_cast<float>(glfwGetTime());
        // Adicionamos um fator aleatório baseado no PhaseOffset para que não atirem todas no exato mesmo milissegundo
        if (currentTime - lastEnemyShotTime >= (ENEMY_SHOT_COOLDOWN + (sin(enemy.PhaseOffset) * 0.3f)))
        {
          EnemyLaser el;
          el.Position = enemy.Position;
          el.Direction = glm::normalize(camera.Position - enemy.Position);
          el.LifeTime = 4.0f;

          activeEnemyLasers.push_back(el);
          // O cooldown global é resetado, mas mitigado pelo offset individual acima
          lastEnemyShotTime = currentTime;
        }
      }

      // --- SISTEMA DE TRANSIÇÃO DE ONDAS ---
      // Se todas as naves da onda atual morreram...
      if (!anyEnemyAlive)
      {
        waveTransitionTimer += deltaTime;
        if (waveTransitionTimer >= WAVE_DELAY)
        {
          currentWave++;
          spawnWave(currentWave);
          waveTransitionTimer = 0.0f;
        }
      }
    }

    // 3. ATUALIZAÇÃO DA FÍSICA DOS LASERS INIMIGOS
    for (auto it = activeEnemyLasers.begin(); it != activeEnemyLasers.end();)
    {
      it->Position += it->Direction * 20.0f * deltaTime;
      it->LifeTime -= deltaTime;

      // DETECÇÃO DE COLISÃO COM O JOGADOR
      float distToPlayer = glm::distance(it->Position, camera.Position);
      if (distToPlayer < 0.8f)
      {
        if (currentGameState == PLAYING)
        {
          player.HP -= 20; // Cada tiro inimigo tira 20 de HP
          // std::cout << "\n[ALERTA] VOCÊ FOI ATINGIDO! HP: " << player.HP << "/100" << std::endl;

          if (player.HP <= 0)
          {
            player.HP = 0;
            player.IsAlive = false;
            currentGameState = GAME_OVER;

            std::cout << "\n===================================" << std::endl;
            std::cout << "            GAME OVER!             " << std::endl;
            std::cout << " Pressione 'R' para tentar de novo " << std::endl;
            std::cout << "===================================\n"
                      << std::endl;
          }
        }

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

    // 4. MOTOR DE FÍSICA DOS LASERS DO JOGADOR (SUB-STEPPING MULTI-ALVO)
    for (auto it = activeLasers.begin(); it != activeLasers.end();)
    {
      bool laserDestroyed = false;
      int subSteps = 3;
      glm::vec3 stepMove = (it->Direction * 35.0f * deltaTime) / (float)subSteps;

      for (int i = 0; i < subSteps; i++)
      {
        it->Position += stepMove;

        float raioColisao = it->IsSpecial ? 3.0f : 0.5f;
        // Checa colisão contra CADA inimigo do vetor
        for (auto &enemy : activeEnemies)
        {
          if (enemy.IsAlive)
          {
            float dist = glm::distance(it->Position, enemy.Position);

            if (dist < (enemy.Radius + raioColisao))
            {
              killCount++;
              killCountToSpecial++;
              // std::cout << "Kills: " << killCount << "/2" << std::endl;

              if (killCountToSpecial >= 5)
              { // Era 2, mudei para 5 conforme sua ideia
                canUseSpecial = true;
                // Não resetar aqui, vamos resetar apenas quando disparar o especial
              }
              // std::cout << "\n[ACERTO] UMA NAVE INIMIGA FOI DESTRUÍDA!" << std::endl;
              score++;
              // std::cout << "SCORE ATUAL: " << score << std::endl;

              spawnExplosion(enemy.Position);
              soloud.play(soundExplosion);
              enemy.IsAlive = false; // Mata este inimigo específico
              laserDestroyed = true;
              break;
            }
          }
        }

        if (killCountToSpecial >= 5)
        {
          canUseSpecial = true;
          killCount = 0; // Reseta o contador
          killCountToSpecial = 0;
          std::cout << "[INFO] ESPECIAL CARREGADO! Pressione 'E' para carregar." << std::endl;
        }

        if (laserDestroyed)
          break;
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

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && canUseSpecial)
    {
      if (!isChargingSpecial)
      {
        isChargingSpecial = true;
        chargeTimer = 0.0f;
        godzillaHandle = soloud.play(soundGodzillaBlast, 10.0f); // Som de início de carga
      }

      chargeTimer += deltaTime;

      // Se atingiu o tempo máximo, dispara
      if (chargeTimer >= CHARGE_TIME)
      {
        dispararLaserEspecial();
        isChargingSpecial = false;
        chargeTimer = 0.0f;
        canUseSpecial = false; // Desativa até ganhar mais kills
        killCountToSpecial = 0;
      }
    }
    else if (isChargingSpecial)
    {
      // Se soltou a tecla 'E' antes dos segundos necessários, cancela o especial
      isChargingSpecial = false;
      chargeTimer = 0.0f;
      stopGodzillaSound(godzillaHandle);
      canUseSpecial = false;
      killCountToSpecial = 0;
    }

    // B. Processamento do carregamento
    if (isChargingSpecial)
    {
      chargeTimer += deltaTime;
      if (chargeTimer >= CHARGE_TIME)
      {
        dispararLaserEspecial();
        isChargingSpecial = false;
        canUseSpecial = false; // Desativa até ganhar mais kills
        std::cout << "[INFO] ESPECIAL DISPARADO!" << std::endl;
        chargeTimer = 0.0f;
      }
    }

    // 5. MOTOR DE FÍSICA DAS PARTÍCULAS
    for (auto it = activeParticles.begin(); it != activeParticles.end();)
    {
      std::string strVet = glm::to_string(it->Color);
      const char *charVet = strVet.c_str();

      std::cout << charVet << std::endl;

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
      // PASSO 2: DESENHAR TODAS AS NAVES INIMIGAS VIVAS
      objectShader.use();
      glm::vec3 luzPosicao(5.0f, 10.0f, 2.0f);
      glUniform3fv(glGetUniformLocation(objectShader.ID, "lightPos"), 1, glm::value_ptr(luzPosicao));
      glUniform3fv(glGetUniformLocation(objectShader.ID, "viewPos"), 1, glm::value_ptr(camera.Position));
      glUniform3f(glGetUniformLocation(objectShader.ID, "lightColor"), 1.0f, 1.0f, 1.0f);

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      for (const auto &enemy : activeEnemies)
      {
        if (!enemy.IsAlive)
          continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, enemy.Position);
        model = glm::rotate(model, glm::radians(80.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        float escalaNave = 0.15f;
        model = glm::scale(model, glm::vec3(escalaNave, escalaNave, escalaNave));

        glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(objectShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        enemyModel.Draw(objectShader);
      }
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

      if (laser.IsSpecial)
      {
        // 1. Aumenta a espessura drasticamente
        glLineWidth(6000.0f);

        // 2. Cor vibrante de plasma
        glUniform3f(glGetUniformLocation(laserShader.ID, "laserColor"), 0.0f, 1.0f, 1.0f);

        glm::mat4 bModel = glm::mat4(190.0f);
        bModel = glm::translate(bModel, laser.Position);

        // 3. ESCALA NO EIXO Z para alongar o laser (Kamehameha longo)
        // O valor 20.0f vai esticar o laser para frente
        bModel = glm::scale(bModel, glm::vec3(100.0f, 100.0f, 100.0f));

        glUniformMatrix4fv(glGetUniformLocation(laserShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(bModel));

        glDrawArrays(GL_LINES, 0, 2);

        // Resetamos a largura
        glLineWidth(3.0f);
        glDrawArrays(GL_LINES, 0, 2);

        // Efeito de partículas ao longo do raio para dar a sensação de energia
        for (int i = 0; i < 5; i++)
        {
          spawnParticle(laser.Position + (laser.Direction * (float)i * 2.0f), glm::vec3(0.1f));
        }
      }
    }

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

    // =================================================================
    // PASSO 6: RENDERIZAR INTERFACE GRÁFICA (HUD 2D)
    // =================================================================
    // Desativamos o Depth Test para que a interface seja desenhada
    // POR CIMA de todo o universo 3D, sem ser cortada por estrelas ou naves.
    glDisable(GL_DEPTH_TEST);

    hudShader.use();

    // Criamos uma matriz ortográfica mapeando diretamente os píxeis da janela
    // Canto esquerdo: 0, Canto direito: SCREEN_WIDTH, Baixo: SCREEN_HEIGHT, Topo: 0
    glm::mat4 projection2D = glm::ortho(0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(hudShader.ID, "projection2D"), 1, GL_FALSE, glm::value_ptr(projection2D));

    glBindVertexArray(hudVAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // -----------------------------------------------------------------
    // A. DESENHAR O FUNDO DA BARRA DE VIDA (MOLDURA PRETA/CINZENTA)
    // -----------------------------------------------------------------
    glm::mat4 hudModel = glm::mat4(1.0f);
    hudModel = glm::translate(hudModel, glm::vec3(30.0f, 30.0f, 0.0f)); // Posição: Margem de 30px do topo esquerdo
    hudModel = glm::scale(hudModel, glm::vec3(200.0f, 25.0f, 1.0f));    // Tamanho: 200px de largura por 25px de altura
    glUniformMatrix4fv(glGetUniformLocation(hudShader.ID, "projection2D"), 1, GL_FALSE, glm::value_ptr(projection2D * hudModel));

    glUniform3f(glGetUniformLocation(hudShader.ID, "hudColor"), 0.15f, 0.15f, 0.15f); // Cor cinzenta escura
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // -----------------------------------------------------------------
    // B. DESENHAR A BARRA DE VIDA PREENCHIDA (DINÂMICA)
    // -----------------------------------------------------------------
    if (player.HP > 0)
    {
      // Calcula a largura proporcional ao HP atual do jogador (ex: 100 HP = 1.0 * 200px = 200px)
      float hpPercent = (float)player.HP / (float)player.MaxHP;
      float barraLargura = 200.0f * hpPercent;

      hudModel = glm::mat4(1.0f);
      hudModel = glm::translate(hudModel, glm::vec3(30.0f, 30.0f, 0.0f));    // Mesma posição de origem
      hudModel = glm::scale(hudModel, glm::vec3(barraLargura, 25.0f, 1.0f)); // Largura encolhe com o dano
      glUniformMatrix4fv(glGetUniformLocation(hudShader.ID, "projection2D"), 1, GL_FALSE, glm::value_ptr(projection2D * hudModel));

      // Efeito Arcade: A barra fica vermelha se a vida estiver abaixo de 30%, senão fica verde brilhante
      if (player.HP <= 30)
      {
        glUniform3f(glGetUniformLocation(hudShader.ID, "hudColor"), 1.0f, 0.0f, 0.0f); // Vermelho Perigo
      }
      else
      {
        glUniform3f(glGetUniformLocation(hudShader.ID, "hudColor"), 0.0f, 0.8f, 0.2f); // Verde Saudável
      }

      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // -----------------------------------------------------------------
    // C. DESENHAR A BARRA DE CARREGAMENTO DO ESPECIAL (NOVO)
    // -----------------------------------------------------------------
    if (isChargingSpecial)
    {
      float chargePercent = chargeTimer / CHARGE_TIME; // 13 segundos definidos
      float barraLargura = 200.0f * chargePercent;

      // Criamos uma matriz de modelo separada para a barra especial
      glm::mat4 specialHudModel = glm::mat4(1.0f);

      // Posição: 30px da esquerda, 65px do topo (abaixo da barra de vida)
      specialHudModel = glm::translate(specialHudModel, glm::vec3(30.0f, 65.0f, 0.0f));
      specialHudModel = glm::scale(specialHudModel, glm::vec3(barraLargura, 10.0f, 1.0f));

      // Aplica a transformação ao Shader
      glUniformMatrix4fv(glGetUniformLocation(hudShader.ID, "projection2D"), 1, GL_FALSE, glm::value_ptr(projection2D * specialHudModel));

      // Cor: Azul Neon (0.0, 0.5, 1.0)
      glUniform3f(glGetUniformLocation(hudShader.ID, "hudColor"), 0.0f, 0.5f, 1.0f);

      // Desenha a barra
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // -----------------------------------------------------------------
    // D. SE ESTIVER EM GAME OVER...
    // -----------------------------------------------------------------

    // -----------------------------------------------------------------
    // C. SE ESTIVER EM GAME OVER: DESENHAR UM PAINEL DE DERROTA
    // -----------------------------------------------------------------
    if (currentGameState == GAME_OVER)
    {

      stopGodzillaSound(godzillaHandle);
      // Desenha um retângulo vermelho translúcido/escuro cobrindo o centro do ecrã
      hudModel = glm::mat4(1.0f);
      hudModel = glm::translate(hudModel, glm::vec3((SCREEN_WIDTH / 2.0f) - 150.0f, (SCREEN_HEIGHT / 2.0f) - 40.0f, 0.0f));
      hudModel = glm::scale(hudModel, glm::vec3(300.0f, 80.0f, 1.0f));
      glUniformMatrix4fv(glGetUniformLocation(hudShader.ID, "projection2D"), 1, GL_FALSE, glm::value_ptr(projection2D * hudModel));

      glUniform3f(glGetUniformLocation(hudShader.ID, "hudColor"), 0.5f, 0.0f, 0.0f); // Vermelho Escuro
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Reativamos o DEPTH TEST para que o renderizador 3D do próximo frame funcione perfeitamente
    glEnable(GL_DEPTH_TEST);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  soloud.deinit();
  glDeleteVertexArrays(1, &hudVAO);
  glDeleteBuffers(1, &hudVBO);
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

  // Se o jogo acabou, o jogador só pode interagir para reiniciar
  if (currentGameState == GAME_OVER)
  {
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
      // --- REINICIAR O JOGO ---
      player.HP = 100;
      player.IsAlive = true;
      score = 0; // Reseta a pontuação
      activeLasers.clear();
      activeEnemyLasers.clear();
      activeParticles.clear();

      enemyTarget.Position = glm::vec3(0.0f, 0.0f, -5.0f);
      enemyTarget.IsAlive = true;

      currentGameState = PLAYING;
      std::cout << "\n===================================" << std::endl;
      std::cout << "        PARTIDA REINICIADA!        " << std::endl;
      std::cout << "===================================\n"
                << std::endl;
    }
    return; // Bloqueia qualquer outra movimentação ou tiro
  }

  // --- MOVIMENTAÇÃO DA CÂMERA (Só roda se estiver VIVO/PLAYING) ---
  float cameraSpeed = static_cast<float>(2.5f * deltaTime);
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
      soloud.play(soundLaser);
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

void spawnWave(int waveNumber)
{
  activeEnemies.clear();
  activeEnemyLasers.clear(); // Limpa tiros antigos para ser justo

  // Fórmula: Começa com 2 naves na Onda 1, e adiciona 1 nave por onda
  int numEnemies = 1 + waveNumber;

  for (int i = 0; i < numEnemies; i++)
  {
    Enemy e;
    // Espalha os inimigos em posições diferentes no eixo X e Y
    float offsetX = (i - (numEnemies - 1) / 2.0f) * 3.5f;
    e.Position = glm::vec3(offsetX, (rand() % 20 - 10) * 0.1f, -15.0f - (i * 2.0f));
    e.Radius = 1.2f;
    e.IsAlive = true;
    e.PhaseOffset = i * 1.5f;                       // Garante que cada nave dance em um ritmo diferente
    e.SpeedMultiplier = 1.0f + (waveNumber * 0.1f); // Ficam mais rápidas a cada onda

    activeEnemies.push_back(e);
  }
  // std::cout << "\n===================================" << std::endl;
  // std::cout << "         INICIANDO ONDA " << currentWave << "         " << std::endl;
  // std::cout << "        PREPARE-SE PARA O COMBATE!  " << std::endl;
  // std::cout << "===================================\n"
  //           << std::endl;
}

void initAudio()
{

  if (soloud.init() != SoLoud::SO_NO_ERROR)
  {
    std::cerr << "Falha ao inicializar o SoLoud!" << std::endl;
  }

  if (soundExplosion.load("audio/explosion.wav") != SoLoud::SO_NO_ERROR)
  {
    std::cerr << "Erro: Nao foi possivel carregar o arquivo de audio!" << std::endl;
  }

  if (soundLaser.load("audio/attack_laser.wav") != SoLoud::SO_NO_ERROR)
  {
    std::cerr << "Erro: Nao foi possivel carregar o arquivo de audio!" << std::endl;
  }

  if (soundGodzillaBlast.load("audio/godzilla_atomic_breath.wav") != SoLoud::SO_NO_ERROR)
  {
    std::cerr << "Erro: Nao foi possivel carregar o arquivo de audio!" << std::endl;
  }
}

void dispararLaserEspecial()
{
  Laser especial;
  especial.Position = camera.Position;
  especial.Direction = camera.Front;
  especial.IsSpecial = true;    // Flag na struct do seu laser
  especial.Damage = 1000;       // Dano massivo
  especial.LifeTime = 10000.0f; // Garanta que ele tenha tempo de vida!
  especial.Scale = 10000.0f;    // Certifique-se de que seu shader lê essa variável
  activeLasers.push_back(especial);
}

void spawnParticle(glm::vec3 position, glm::vec3 velocity)
{
  Particle p;
  p.Position = position;
  p.Velocity = velocity;
  p.LifeTime = 1.0f;                           // Duração da partícula no ar
  p.Color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); // Azul Ciano para combinar com a bola
  activeParticles.push_back(p);
}

void stopGodzillaSound(unsigned int godzillaHandle)
{
  if (godzillaHandle != 0)
  {
    soloud.stop(godzillaHandle);
    godzillaHandle = 0;
  }
}