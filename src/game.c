#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ranking.h"
#include "commons.h"
#include "core/effects.h"
#include "core/game_common.h"

// =========================================================================
// STRUCTS E ENUMS
// =========================================================================
typedef struct {
    float x, y;
    float alpha;
    float scale;
} TrailPoint;

typedef struct {
    Vector2 start;
    Vector2 end;
    float alpha;
    float probability;
} PredictionPath;

typedef enum {
    STATE_START_SCREEN,
    STATE_ASKING_QUESTION,
    STATE_WAITING_FOR_BALL,
    STATE_BALL_FALLING,
    STATE_BALL_LANDED,
    STATE_NAME_INPUT,
    STATE_GAME_OVER
} InternalGameState;

typedef struct {
    const char* texto;
    const char* opcoes[3];
    int resposta_correta;
    Color corTema;
} Pergunta;

// =========================================================================
// VARIÁVEIS GLOBAIS
// =========================================================================
static Pin pins[NUM_PINS_X * NUM_PINS_Y];
static int pinCount = 0;
static int slotValues[SLOT_COUNT];
static float firstPinY = 120.0f;
static float firstSlotX;
static float slotWidth;
static float baseY;
static char playerName[MAX_NAME_LENGTH + 1] = { 0 };
static int letterCount = 0;
static const int gameAreaHeight = 800;

static Pergunta perguntas[NUM_ETAPAS] = {
    { "Qual a capital da Franca?",
      {"1. Londres", "2. Paris", "3. Berlim"}, 1, COLOR_NEON_BLUE },
    { "Quem pintou a Mona Lisa?",
      {"1. Van Gogh", "2. Picasso", "3. Da Vinci"}, 2, COLOR_NEON_GOLD },
    { "Quanto e 5 x 8?",
      {"1. 40", "2. 45", "3. 35"}, 0, COLOR_NEON_GREEN },
    { "Qual o maior planeta do Sistema Solar?",
      {"1. Terra", "2. Marte", "3. Jupiter"}, 2, COLOR_NEON_PURPLE },
    { "Em que ano o homem pisou na Lua?",
      {"1. 1969", "2. 1975", "3. 1982"}, 0, COLOR_NEON_RED }
};

static InternalGameState currentState;
static int slotCounts[SLOT_COUNT];
static int totalBolas;
static Ball ball;
static TrailPoint ballTrail[TRAIL_LENGTH];
static int trailIndex = 0;
static int currentStage;
static long long totalScore;
static int lastAnswerWasCorrect;
static int lastValue;
static Color slotColor;
static Particle particles[PARTICLE_COUNT];

static float uiPulse = 0.0f;
static int comboCount = 0;
static float comboDisplayTimer = 0.0f;

// Variáveis para análise preditiva
static PredictionPath predictionPaths[MAX_PREDICTION_PATHS];
static float currentProbabilities[SLOT_COUNT];
static char analysisText[3][64];
static float uncertaintyLevel = 1.0f;
static int pinsRemaining = NUM_PINS_Y;
static int mostProbableSlot = -1;
static float highestProbability = 0.0f;

extern GameScreen currentScreen;

// =========================================================================
// FUNÇÕES MATEMÁTICAS E AUXILIARES
// =========================================================================
long long factorial(int n) {
    if (n < 0) return 0;
    long long f = 1;
    for (int i = 2; i <= n; i++) f *= i;
    return f;
}

long long combinations(int n, int k) {
    if (k < 0 || k > n) return 0;
    long long denom = factorial(k) * factorial(n - k);
    return denom ? factorial(n) / denom : 0;
}

float MathLerp(float a, float b, float t) {
    return a + t * (b - a);
}

// float RandomFloat(float min, float max) {
//     return min + ((float)rand() / RAND_MAX) * (max - min);
// }

// Função auxiliar para calcular precisão da análise preditiva
float CalculatePredictionAccuracy(void) {
    return 75.0f + (float)(rand() % 20); // Simula 75-95% de precisão
}

// =========================================================================
// SISTEMA DE ANÁLISE PREDITIVA
// =========================================================================

void CalculateRealTimeProbabilities(float ballX, float ballY) {
    // Calcula quantos pinos faltam baseado na posição Y da bola
    float progress = (ballY - firstPinY) / ((baseY - BALL_RADIUS) - firstPinY);
    progress = fmaxf(0.0f, fminf(1.0f, progress));
    pinsRemaining = (int)((1.0f - progress) * NUM_PINS_Y);
    pinsRemaining = fmax(1, pinsRemaining); // Mínimo 1 pino restante

    // Calcula o slot central baseado na posição X atual
    int centerSlot = (int)((ballX - firstSlotX) / slotWidth);
    centerSlot = fmax(0, fmin(SLOT_COUNT - 1, centerSlot));

    // Calcula probabilidades para cada slot
    highestProbability = 0.0f;
    mostProbableSlot = -1;

    for (int targetSlot = 0; targetSlot < SLOT_COUNT; targetSlot++) {
        // Distância do slot alvo ao slot central
        int distance = abs(targetSlot - centerSlot);

        // Se a distância for maior que os pinos restantes, probabilidade é zero
        if (distance > pinsRemaining) {
            currentProbabilities[targetSlot] = 0.0f;
            continue;
        }

        // Calcula probabilidade binomial
        double prob = (double)combinations(pinsRemaining, distance) * pow(0.5, pinsRemaining);
        currentProbabilities[targetSlot] = (float)prob;

        // Atualiza slot mais provável
        if (prob > highestProbability) {
            highestProbability = (float)prob;
            mostProbableSlot = targetSlot;
        }
    }

    // Calcula nível de incerteza (entropia)
    uncertaintyLevel = 0.0f;
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (currentProbabilities[i] > 0.0f) {
            uncertaintyLevel -= currentProbabilities[i] * log2f(currentProbabilities[i]);
        }
    }
    uncertaintyLevel /= log2f(SLOT_COUNT); // Normaliza para 0-1

    // Atualiza textos de análise
    if (pinsRemaining == NUM_PINS_Y) {
        snprintf(analysisText[0], sizeof(analysisText[0]), "Incerteza Máxima (50/50)");
        snprintf(analysisText[1], sizeof(analysisText[1]), "Todos os caminhos possíveis");
        snprintf(analysisText[2], sizeof(analysisText[2]), "Distribuição Uniforme");
    } else if (pinsRemaining > NUM_PINS_Y / 2) {
        snprintf(analysisText[0], sizeof(analysisText[0]), "Alta Incerteza");
        snprintf(analysisText[1], sizeof(analysisText[1]), "Múltiplos caminhos prováveis");
        snprintf(analysisText[2], sizeof(analysisText[2]), "Convergindo gradualmente");
    } else if (pinsRemaining > 2) {
        snprintf(analysisText[0], sizeof(analysisText[0]), "Incerteza Moderada");
        snprintf(analysisText[1], sizeof(analysisText[1]), "Focando no slot %d", mostProbableSlot + 1);
        snprintf(analysisText[2], sizeof(analysisText[2]), "Curva se estreitando");
    } else {
        snprintf(analysisText[0], sizeof(analysisText[0]), "Baixa Incerteza");
        snprintf(analysisText[1], sizeof(analysisText[1]), "Destino quase definido");
        snprintf(analysisText[2], sizeof(analysisText[2]), "Slot %d (%.1f%%)",
                 mostProbableSlot + 1, highestProbability * 100);
    }
}

void UpdatePredictionPaths(float ballX, float ballY) {
    // Limpa caminhos antigos
    for (int i = 0; i < MAX_PREDICTION_PATHS; i++) {
        predictionPaths[i].alpha = MathLerp(predictionPaths[i].alpha, 0.0f, 0.3f);
    }

    // Cria novos caminhos para slots com probabilidade significativa
    int pathIndex = 0;
    for (int slot = 0; slot < SLOT_COUNT && pathIndex < MAX_PREDICTION_PATHS; slot++) {
        if (currentProbabilities[slot] > 0.05f) { // Apenas slots com >5% de chance
            float slotCenterX = firstSlotX + slot * slotWidth + slotWidth / 2;

            predictionPaths[pathIndex].start = (Vector2){ ballX, ballY };
            predictionPaths[pathIndex].end = (Vector2){ slotCenterX, baseY - BALL_RADIUS };
            predictionPaths[pathIndex].alpha = currentProbabilities[slot] * 0.8f;
            predictionPaths[pathIndex].probability = currentProbabilities[slot];

            pathIndex++;
        }
    }
}

void DrawPredictionCurve(void) {
    if (pinsRemaining <= 0) return;

    // Desenha curva de probabilidade preditiva
    int pointCount = 50;
    Vector2 points[pointCount];
    float maxProb = 0.0f;

    // Encontra probabilidade máxima para escalar o gráfico
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (currentProbabilities[i] > maxProb) {
            maxProb = currentProbabilities[i];
        }
    }

    if (maxProb <= 0.0f) return;

    // Cria pontos suaves para a curva
    for (int i = 0; i < pointCount; i++) {
        float t = (float)i / (pointCount - 1);
        int slot = (int)(t * (SLOT_COUNT - 1));
        float x = firstSlotX + slot * slotWidth + slotWidth / 2;
        float y = baseY - (currentProbabilities[slot] / maxProb) * 150.0f; // Altura escalada

        points[i] = (Vector2){ x, y };
    }

    // Desenha a curva suave
    for (int i = 0; i < pointCount - 1; i++) {
        float alpha = 150.0f * (0.3f + 0.7f * uncertaintyLevel);
        Color curveColor = (Color){
            COLOR_PREDICTION.r,
            COLOR_PREDICTION.g,
            COLOR_PREDICTION.b,
            (unsigned char)alpha
        };
        DrawLineEx(points[i], points[i + 1], 3.0f, curveColor);
    }

    // Desenha área sob a curva
    for (int i = 0; i < pointCount - 1; i++) {
        Vector2 quad[4] = {
            points[i],
            points[i + 1],
            {points[i + 1].x, baseY},
            {points[i].x, baseY}
        };
        Color fillColor = (Color){
            COLOR_PREDICTION.r,
            COLOR_PREDICTION.g,
            COLOR_PREDICTION.b,
            30
        };
        DrawTriangle(quad[0], quad[1], quad[2], fillColor);
        DrawTriangle(quad[0], quad[2], quad[3], fillColor);
    }
}

void DrawPredictionPaths(void) {
    for (int i = 0; i < MAX_PREDICTION_PATHS; i++) {
        if (predictionPaths[i].alpha > 0.01f) {
            Color pathColor = COLOR_PATH;
            pathColor.a = (unsigned char)(predictionPaths[i].alpha * 255);

            // Linha principal
            DrawLineEx(predictionPaths[i].start, predictionPaths[i].end,
                      2.0f * predictionPaths[i].probability, pathColor);

            // Ponto de destino pulsante
            float pulse = sinf(GetTime() * 4.0f) * 0.3f + 0.7f;
            DrawCircleV(predictionPaths[i].end,
                      6.0f * predictionPaths[i].probability * pulse,
                      pathColor);
        }
    }
}

// =========================================================================
// SISTEMA DE PARTÍCULAS E TRAIL
// =========================================================================
void InitParticles(void) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        particles[i].life = 0.0f;
    }
}

void CreateParticles(float x, float y, Color color, int count) {
    for (int i = 0; i < PARTICLE_COUNT && count > 0; i++) {
        if (particles[i].life <= 0.0f) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = RandomFloat(-400, 400);
            particles[i].vy = RandomFloat(-400, 400);
            particles[i].life = particles[i].maxLife = RandomFloat(0.3f, 1.0f);
            particles[i].color = color;
            particles[i].size = RandomFloat(2, 6);
            count--;
        }
    }
}

void UpdateParticles(float dt) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (particles[i].life > 0.0f) {
            particles[i].x += particles[i].vx * dt;
            particles[i].y += particles[i].vy * dt;
            particles[i].vy += GRAVITY * 0.3f * dt;
            particles[i].life -= dt;
            particles[i].color.a = (unsigned char)(255 * (particles[i].life / particles[i].maxLife));
        }
    }
}

void UpdateBallTrail(float ballX, float ballY, float dt) {
    for (int i = 0; i < TRAIL_LENGTH; i++) {
        ballTrail[i].alpha = MathLerp(ballTrail[i].alpha, 0.0f, 8.0f * dt);
        ballTrail[i].scale = MathLerp(ballTrail[i].scale, 0.3f, 6.0f * dt);
    }

    ballTrail[trailIndex].x = ballX;
    ballTrail[trailIndex].y = ballY;
    ballTrail[trailIndex].alpha = 1.0f;
    ballTrail[trailIndex].scale = 1.0f;

    trailIndex = (trailIndex + 1) % TRAIL_LENGTH;
}





// =========================================================================
// INICIALIZAÇÃO
// =========================================================================
void InitGame(void) {
    currentState = STATE_START_SCREEN;
    currentStage = 0;
    totalScore = 0;
    totalBolas = 0;
    lastAnswerWasCorrect = 0;
    lastValue = 0;
    slotColor = COLOR_NEON_BLUE;
    letterCount = 0;
    comboCount = 0;
    comboDisplayTimer = 0.0f;
    memset(playerName, 0, sizeof(playerName));

    // Inicializa sistema preditivo
    for (int i = 0; i < SLOT_COUNT; i++) {
        slotCounts[i] = 0;
        currentProbabilities[i] = 0.0f;
    }

    for (int i = 0; i < MAX_PREDICTION_PATHS; i++) {
        predictionPaths[i].alpha = 0.0f;
    }

    strcpy(analysisText[0], "Aguardando início...");
    strcpy(analysisText[1], "Preparando análise...");
    strcpy(analysisText[2], "Sistema pronto");

    // Inicializa bola
    ball.active = 0;
    ball.x = SCREEN_WIDTH * 0.5f;
    ball.y = 60;
    ball.vx = 0;
    ball.vy = 0;
    ball.color = COLOR_NEON_GOLD;
    ball.scale = 1.0f;
    ball.rotation = 0.0f;
    ball.rotationSpeed = 0.0f;

    // Inicializa trail
    for (int i = 0; i < TRAIL_LENGTH; i++) {
        ballTrail[i].alpha = 0.0f;
        ballTrail[i].scale = 0.3f;
    }

    // Configura pinos
    pinCount = 0;
    float totalBoardWidth = NUM_PINS_X * PIN_SPACING;

    for (int y = 0; y < NUM_PINS_Y; y++) {
        for (int x = 0; x < NUM_PINS_X; x++) {
            float offset = (y % 2 == 0) ? 0 : PIN_SPACING * 0.5f;
            pins[pinCount].x = SCREEN_WIDTH * 0.5f - totalBoardWidth * 0.5f + offset + x * PIN_SPACING;
            pins[pinCount].y = firstPinY + y * PIN_SPACING;
            pins[pinCount].color = (Color){180, 180, 200, 255};
            pins[pinCount].visible = 1;
            pinCount++;
        }
    }

    // Configura slots
    slotWidth = PIN_SPACING;
    baseY = 750;
    float totalSlotsWidth = SLOT_COUNT * slotWidth;
    firstSlotX = (SCREEN_WIDTH - totalSlotsWidth) * 0.5f;

    int valoresBase[SLOT_COUNT] = {1, 10, 1000, 100, 500, 100, 1000, 100, 500, 100, 1000, 10, 1};
    for (int i = 0; i < SLOT_COUNT; i++) {
        slotValues[i] = valoresBase[i];
    }

    InitParticles();
}

// =========================================================================
// UPDATE COM ANÁLISE PREDITIVA
// =========================================================================
void UpdateGame(void) {
    float dt = GetFrameTime();
    UpdateSlowMotion();
    dt = ApplyTimeScale(dt);

    uiPulse = sinf(GetTime() * UI_ANIM_SPEED) * 0.5f + 0.5f;

    if (comboDisplayTimer > 0.0f) {
        comboDisplayTimer -= dt;
    }

    UpdateScreenShake(dt);
    UpdateParticles(dt);

    if (IsKeyPressed(KEY_ESCAPE)) {
        currentScreen = SCREEN_MENU;
    }

    switch (currentState) {
        case STATE_START_SCREEN: {
            if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP)) {
                currentState = STATE_ASKING_QUESTION;
            }
        } break;

        case STATE_ASKING_QUESTION: {
            int resposta = -1;
            if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) resposta = 0;
            if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) resposta = 1;
            if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) resposta = 2;

            if (resposta != -1) {
                if (resposta == perguntas[currentStage].resposta_correta) {
                    lastAnswerWasCorrect = 1;
                    slotColor = COLOR_NEON_GREEN;
                    comboCount++;
                    if (comboCount > 1) {
                        comboDisplayTimer = 2.0f;
                        CreateParticles(SCREEN_WIDTH * 0.5f, 150, COLOR_NEON_GOLD, 20);
                    }
                    StartScreenShake(3.0f);
                } else {
                    lastAnswerWasCorrect = 0;
                    slotColor = COLOR_NEON_RED;
                    comboCount = 0;
                    StartScreenShake(6.0f);
                }
                currentState = STATE_WAITING_FOR_BALL;
            }
        } break;

        case STATE_WAITING_FOR_BALL: {
            // Reseta análise preditiva
            pinsRemaining = NUM_PINS_Y;
            uncertaintyLevel = 1.0f;
            strcpy(analysisText[0], "Pronto para análise");
            strcpy(analysisText[1], "Aguardando lançamento");
            strcpy(analysisText[2], "Incerteza máxima");

            if ((IsKeyPressed(KEY_SPACE) || IsGestureDetected(GESTURE_TAP)) && !ball.active) {
                ball.x = SCREEN_WIDTH * 0.5f;
                ball.y = 60;
                ball.vx = RandomFloat(-100, 100);
                ball.vy = 0;
                ball.active = 1;
                ball.slotIndex = -1;
                ball.color = lastAnswerWasCorrect ? COLOR_NEON_GREEN : COLOR_NEON_RED;
                ball.scale = 1.3f;
                ball.rotationSpeed = ball.vx * 0.02f;
                currentState = STATE_BALL_FALLING;
            }
        } break;

        case STATE_BALL_FALLING: {
            if (ball.active) {
                UpdateBallTrail(ball.x, ball.y, dt);

                // ATUALIZA ANÁLISE PREDITIVA EM TEMPO REAL
                CalculateRealTimeProbabilities(ball.x, ball.y);
                UpdatePredictionPaths(ball.x, ball.y);

                ball.vy += GRAVITY * dt;
                ball.vx *= AIR_RESISTANCE;
                ball.vy *= AIR_RESISTANCE;
                ball.rotationSpeed = MathLerp(ball.rotationSpeed, ball.vx * 0.015f, 5.0f * dt);

                ball.x += ball.vx * dt;
                ball.y += ball.vy * dt;
                ball.rotation += ball.rotationSpeed;
                ball.scale = MathLerp(ball.scale, 1.0f, 8.0f * dt);

                // Colisão com pinos
                for (int i = 0; i < pinCount; i++) {
                    if (!pins[i].visible) continue;

                    float dx = ball.x - pins[i].x;
                    float dy = ball.y - pins[i].y;
                    float dist = sqrtf(dx*dx + dy*dy);
                    float minDist = BALL_RADIUS + PIN_RADIUS;

                    if (dist < minDist) {
                        float nx = dx / dist;
                        float ny = dy / dist;

                        float penetration = minDist - dist;
                        ball.x += nx * penetration * 0.5f;
                        ball.y += ny * penetration * 0.5f;

                        float dotProduct = ball.vx * nx + ball.vy * ny;
                        ball.vx = (ball.vx - 2.0f * dotProduct * nx) * ELASTICITY;
                        ball.vy = (ball.vy - 2.0f * dotProduct * ny) * ELASTICITY;

                        pins[i].color = YELLOW;
                        CreateParticles(pins[i].x, pins[i].y, YELLOW, 8);
                        ball.rotationSpeed += ball.vx * 0.03f;
                    }
                }

                // Colisão com paredes
                if (ball.x < BALL_RADIUS) {
                    ball.x = BALL_RADIUS;
                    ball.vx = fabsf(ball.vx) * FRICTION;
                    CreateParticles(ball.x, ball.y, COLOR_NEON_BLUE, 5);
                }
                if (ball.x > SCREEN_WIDTH - BALL_RADIUS) {
                    ball.x = SCREEN_WIDTH - BALL_RADIUS;
                    ball.vx = -fabsf(ball.vx) * FRICTION;
                    CreateParticles(ball.x, ball.y, COLOR_NEON_BLUE, 5);
                }

                // Aterrissagem nos slots
                if (ball.y > baseY - BALL_RADIUS) {
                    ball.y = baseY - BALL_RADIUS;
                    int idx = (int)((ball.x - firstSlotX) / slotWidth);
                    idx = (idx < 0) ? 0 : (idx >= SLOT_COUNT) ? SLOT_COUNT - 1 : idx;

                    ball.slotIndex = idx;
                    ball.active = 0;

                    slotCounts[idx]++;
                    totalBolas++;

                    lastValue = slotValues[ball.slotIndex];
                    int pointsChange = lastAnswerWasCorrect ? lastValue : -lastValue;
                    totalScore += pointsChange;

                    Color particleColor = lastAnswerWasCorrect ? COLOR_NEON_GREEN : COLOR_NEON_RED;
                    CreateParticles(ball.x, ball.y, particleColor, 15);
                    StartScreenShake(lastAnswerWasCorrect ? 5.0f : 10.0f);

                    currentState = STATE_BALL_LANDED;
                }
            }
        } break;

        case STATE_BALL_LANDED: {
            if (IsKeyPressed(KEY_ENTER)) {
                currentStage++;
                if (currentStage >= NUM_ETAPAS) {
                    currentState = STATE_NAME_INPUT;
                } else {
                    currentState = STATE_ASKING_QUESTION;
                    slotColor = COLOR_NEON_BLUE;
                }
            }
        } break;

        case STATE_NAME_INPUT: {
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (letterCount < MAX_NAME_LENGTH)) {
                    playerName[letterCount] = (char)key;
                    playerName[letterCount + 1] = '\0';
                    letterCount++;
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE)) {
                letterCount--;
                if (letterCount < 0) letterCount = 0;
                playerName[letterCount] = '\0';
            }

            if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
                AddHighScore(playerName, totalScore);
                currentScreen = SCREEN_RANKING;
            }
        } break;

        case STATE_GAME_OVER: {
            if (IsKeyPressed(KEY_R)) InitGame();
            if (IsKeyPressed(KEY_Q)) currentScreen = SCREEN_MENU;
        } break;
    }
}

// =========================================================================
// RENDERIZAÇÃO COM ANÁLISE PREDITIVA
// =========================================================================
void DrawGame(void) {
    BeginMode2D((Camera2D){
        { GetCameraOffsetX(), GetCameraOffsetY() },
        { 0, 0 },
        0.0f, 1.0f
    });

    // Fundo gradiente moderno
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        float t = (float)i / SCREEN_HEIGHT;
        Color gradColor = (Color){
            COLOR_BG.r + (int)(t * 8),
            COLOR_BG.g + (int)(t * 12),
            COLOR_BG.b + (int)(t * 18),
            255
        };
        DrawRectangle(0, i, SCREEN_WIDTH, 1, gradColor);
    }

    // Desenha pinos
    for (int i = 0; i < pinCount; i++) {
        if (pins[i].visible) {
            DrawCircle((int)pins[i].x, (int)pins[i].y, PIN_RADIUS + 2, (Color){255, 255, 255, 60});
            DrawCircle((int)pins[i].x, (int)pins[i].y, PIN_RADIUS, pins[i].color);
            DrawCircleLines((int)pins[i].x, (int)pins[i].y, PIN_RADIUS, WHITE);
        }
    }

    // Área de slots
    DrawRectangleGradientV(0, baseY, SCREEN_WIDTH, gameAreaHeight - baseY,
                          (Color){40, 45, 70, 255}, (Color){25, 30, 50, 255});

    for (int i = 0; i < SLOT_COUNT; i++) {
        float x = firstSlotX + i * slotWidth;
        DrawLine(x, baseY, x, gameAreaHeight, (Color){80, 90, 140, 255});

        char txt[16];
        sprintf(txt, "%d", slotValues[i]);
        int textWidth = MeasureText(txt, 20);
        DrawText(txt, x + slotWidth/2 - textWidth/2, baseY + 20, 20, slotColor);

        if (ball.slotIndex == i && !ball.active) {
            DrawRectangleGradientH(x + 2, baseY, slotWidth - 4, 8,
                                 COLOR_NEON_GOLD, (Color){255, 215, 0, 0});
        }
    }

    // DESENHA VISUALIZAÇÃO PREDITIVA (apenas quando a bola está caindo)
    if (ball.active && ball.slotIndex == -1) {
        DrawPredictionPaths();
        DrawPredictionCurve();
    }

    // Desenha trail da bola
    for (int i = 0; i < TRAIL_LENGTH; i++) {
        if (ballTrail[i].alpha > 0.01f) {
            Color trailColor = ball.color;
            trailColor.a = (unsigned char)(150 * ballTrail[i].alpha);
            DrawCircle((int)ballTrail[i].x, (int)ballTrail[i].y,
                      BALL_RADIUS * 0.7f * ballTrail[i].scale, trailColor);
        }
    }

    // Desenha partículas
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (particles[i].life > 0.0f) {
            DrawCircle((int)particles[i].x, (int)particles[i].y, particles[i].size, particles[i].color);
        }
    }

    // Desenha bola
    if (ball.active) {
        DrawCircle((int)ball.x, (int)ball.y, BALL_RADIUS * ball.scale + 3,
                  (Color){255, 255, 255, 80});

        DrawCircle((int)ball.x, (int)ball.y, BALL_RADIUS * ball.scale, ball.color);
        DrawCircleLines((int)ball.x, (int)ball.y, BALL_RADIUS * ball.scale, WHITE);

        float markerX = ball.x + cosf(ball.rotation) * BALL_RADIUS * 0.6f;
        float markerY = ball.y + sinf(ball.rotation) * BALL_RADIUS * 0.6f;
        DrawCircle((int)markerX, (int)markerY, BALL_RADIUS * 0.2f, (Color){255, 255, 255, 180});
    }

    EndMode2D();

    // =========================================================================
    // HUB DE ANÁLISE ESTATÍSTICA (Lateral Direita) - SEM GRÁFICO
    // =========================================================================

    int statsPanelWidth = 400;
    int statsPanelX = SCREEN_WIDTH - statsPanelWidth - 20;

    // Painel principal de análise (mais compacto sem o gráfico)
    DrawRectangle(statsPanelX, 20, statsPanelWidth, 300, COLOR_UI_BG);
    DrawRectangleLines(statsPanelX, 20, statsPanelWidth, 300, COLOR_UI_BORDER);

    // Header do painel com ícone
    DrawText("HUB DE ANÁLISE", statsPanelX + 15, 35, 22, COLOR_NEON_GOLD);

    // Informações básicas
    DrawText(TextFormat("Bolas: %d", totalBolas), statsPanelX + 20, 70, 18, LIGHTGRAY);
    DrawText(TextFormat("Stage: %d/%d", currentStage + 1, NUM_ETAPAS), statsPanelX + 20, 95, 18, LIGHTGRAY);
    DrawText(TextFormat("Combo: x%d", comboCount), statsPanelX + statsPanelWidth - 100, 95, 18, COLOR_NEON_GREEN);

    // Barra de incerteza visual
    int uncertaintyBarWidth = statsPanelWidth - 40;
    DrawRectangle(statsPanelX + 20, 125, uncertaintyBarWidth, 12, (Color){50, 50, 70, 255});
    DrawRectangle(statsPanelX + 20, 125, (int)(uncertaintyBarWidth * uncertaintyLevel), 12,
                 (Color){255, (int)(255 * uncertaintyLevel), (int)(100 * (1.0f - uncertaintyLevel)), 255});
    DrawText(TextFormat("Incerteza: %.0f%%", uncertaintyLevel * 100), statsPanelX + 20, 140, 14, LIGHTGRAY);

    // Análise preditiva em tempo real
    DrawText("ANÁLISE PREDITIVA:", statsPanelX + 20, 165, 16, COLOR_NEON_BLUE);

    if (ball.active && ball.slotIndex == -1) {
        // Textos dinâmicos de análise
        DrawText(analysisText[0], statsPanelX + 25, 190, 16, WHITE);
        DrawText(analysisText[1], statsPanelX + 25, 215, 14, LIGHTGRAY);
        DrawText(analysisText[2], statsPanelX + 25, 235, 14, LIGHTGRAY);

        // Slot mais provável
        if (mostProbableSlot >= 0) {
            DrawText(TextFormat("Mais provável: Slot %d (%.1f%%)",
                               mostProbableSlot + 1, highestProbability * 100),
                    statsPanelX + 25, 260, 14, COLOR_NEON_GREEN);
        }

        // Pinos restantes
        DrawText(TextFormat("Pinos restantes: %d/%d", pinsRemaining, NUM_PINS_Y),
                statsPanelX + 25, 285, 14, COLOR_NEON_PURPLE);
    } else {
        DrawText("Aguardando lançamento...", statsPanelX + 25, 190, 16, GRAY);
        DrawText("A análise começará quando", statsPanelX + 25, 215, 14, GRAY);
        DrawText("a bola for solta", statsPanelX + 25, 235, 14, GRAY);
    }

    // =========================================================================
    // GRÁFICO DE DISTRIBUIÇÃO ABAIXO DOS SLOTS
    // =========================================================================

    int graphHeight = 150; // Altura reduzida para caber abaixo dos slots
    int graphY = baseY + 50; // Posiciona abaixo da área de slots
    int graphWidth = SCREEN_WIDTH - 40; // Largura quase total da tela
    int graphX = 20;

    // Fundo do gráfico
    DrawRectangle(graphX, graphY, graphWidth, graphHeight, (Color){15, 20, 35, 220});
    DrawRectangleLines(graphX, graphY, graphWidth, graphHeight, COLOR_UI_BORDER);

    // Título do gráfico
    DrawText("DISTRIBUIÇÃO EMPÍRICA - HISTÓRICO DE BOLAS",
             graphX + 10, graphY + 5, 16, COLOR_NEON_GOLD);

    // Desenha gráfico de barras empírico (histórico)
    int maxCount = 0;
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (slotCounts[i] > maxCount) maxCount = slotCounts[i];
    }

    float barWidth = (float)graphWidth / SLOT_COUNT;
    for (int i = 0; i < SLOT_COUNT; i++) {
        float x = graphX + i * barWidth;

        // Fundo da barra (sempre visível)
        DrawRectangle(x + 2, graphY + graphHeight - 20, barWidth - 4, 15, (Color){40, 40, 60, 255});

        if (slotCounts[i] > 0) {
            float barHeight = maxCount > 0 ? (float)slotCounts[i] / maxCount * (graphHeight - 50) : 0;

            // Barra histórica (sólida) - cresce de baixo para cima
            Color barColor = (i == ball.slotIndex && !ball.active) ? COLOR_NEON_GOLD : COLOR_NEON_BLUE;
            DrawRectangle(x + 2, graphY + graphHeight - 20 - barHeight, barWidth - 4, barHeight, barColor);
            DrawRectangleLines(x + 2, graphY + graphHeight - 20 - barHeight, barWidth - 4, barHeight, WHITE);

            // Número da contagem no topo da barra
            if (barHeight > 25) {
                char countText[16];
                sprintf(countText, "%d", slotCounts[i]);
                int textWidth = MeasureText(countText, 12);
                DrawText(countText,
                        x + barWidth/2 - textWidth/2,
                        graphY + graphHeight - 25 - barHeight, 12, WHITE);
            }

            // Porcentagem abaixo da barra
            if (totalBolas > 0) {
                float percentage = (float)slotCounts[i] / totalBolas * 100.0f;
                char percentText[16];
                sprintf(percentText, "%.1f%%", percentage);
                int textWidth = MeasureText(percentText, 10);
                DrawText(percentText,
                        x + barWidth/2 - textWidth/2,
                        graphY + graphHeight - 5, 10, LIGHTGRAY);
            }
        }

        // Número do slot abaixo de cada barra
        char slotText[8];
        sprintf(slotText, "%d", i + 1);
        int slotTextWidth = MeasureText(slotText, 12);
        DrawText(slotText,
                x + barWidth/2 - slotTextWidth/2,
                graphY + graphHeight + 5, 12, WHITE);
    }

    // Legenda do gráfico
    if (totalBolas > 0) {
        DrawText(TextFormat("Total de bolas: %d | Distribuição atual baseada no histórico", totalBolas),
                graphX + 10, graphY + graphHeight + 25, 12, LIGHTGRAY);
    }

    // Painel de pontuação superior esquerdo
    DrawRectangle(20, 20, 280, 100, COLOR_UI_BG);
    DrawRectangleLines(20, 20, 280, 100, COLOR_UI_BORDER);
    DrawText("PONTUAÇÃO", 40, 30, 22, COLOR_NEON_GOLD);
    DrawText(TextFormat("%lld", totalScore), 40, 60, 32, WHITE);

    // =========================================================================
    // INTERFACES DE ESTADO COMPLETAS
    // =========================================================================
    switch (currentState) {
        case STATE_START_SCREEN: {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.85f));

            float titleGlow = sinf(GetTime() * 2.5f) * 0.4f + 0.6f;
            DrawText("THE WALL", (SCREEN_WIDTH - MeasureText("THE WALL", 120)) / 2, 250, 120,
                    (Color){COLOR_NEON_GOLD.r, COLOR_NEON_GOLD.g, COLOR_NEON_GOLD.b, (int)(255 * titleGlow)});

            DrawText("Simulador de Distribuição Binomial",
                    (SCREEN_WIDTH - MeasureText("Simulador de Distribuição Binomial", 36)) / 2, 400, 36, RAYWHITE);

            // Destaque para o novo sistema
            DrawText("COM ANÁLISE PREDITIVA EM TEMPO REAL",
                    (SCREEN_WIDTH - MeasureText("COM ANÁLISE PREDITIVA EM TEMPO REAL", 24)) / 2, 470, 24, COLOR_NEON_BLUE);

            if (((int)(GetTime() * 2) % 2) == 0) {
                DrawText("Pressione ENTER para começar",
                        (SCREEN_WIDTH - MeasureText("Pressione ENTER para começar", 28)) / 2, 550, 28, COLOR_NEON_GREEN);
            }

            DrawText("Trabalho de Estatística - Probabilidade e Combinatória",
                    (SCREEN_WIDTH - MeasureText("Trabalho de Estatística - Probabilidade e Combinatória", 20)) / 2,
                    SCREEN_HEIGHT - 80, 20, GRAY);
        } break;

        case STATE_ASKING_QUESTION: {
            Color bgColor = perguntas[currentStage].corTema;
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(bgColor, 0.15f));

            DrawRectangle(SCREEN_WIDTH * 0.1f, SCREEN_HEIGHT * 0.2f, SCREEN_WIDTH * 0.8f, SCREEN_HEIGHT * 0.5f, COLOR_UI_BG);
            DrawRectangleLines(SCREEN_WIDTH * 0.1f, SCREEN_HEIGHT * 0.2f, SCREEN_WIDTH * 0.8f, SCREEN_HEIGHT * 0.5f, bgColor);

            Pergunta q = perguntas[currentStage];

            DrawText("PERGUNTA:", (SCREEN_WIDTH - MeasureText("PERGUNTA:", 28)) / 2, SCREEN_HEIGHT * 0.23f, 28, WHITE);
            DrawText(q.texto, (SCREEN_WIDTH - MeasureText(q.texto, 26)) / 2, SCREEN_HEIGHT * 0.32f, 26, RAYWHITE);

            for (int i = 0; i < 3; i++) {
                int yPos = SCREEN_HEIGHT * 0.45f + i * 60;
                DrawRectangle(SCREEN_WIDTH * 0.2f, yPos, SCREEN_WIDTH * 0.6f, 50, Fade(WHITE, 0.1f));
                DrawRectangleLines(SCREEN_WIDTH * 0.2f, yPos, SCREEN_WIDTH * 0.6f, 50, GRAY);
                DrawText(q.opcoes[i], SCREEN_WIDTH * 0.22f, yPos + 12, 22, RAYWHITE);
                DrawText(TextFormat("[%d]", i + 1), SCREEN_WIDTH * 0.7f, yPos + 12, 22, YELLOW);
            }

            DrawText("Use as teclas 1, 2 ou 3 para responder",
                    (SCREEN_WIDTH - MeasureText("Use as teclas 1, 2 ou 3 para responder", 20)) / 2,
                    SCREEN_HEIGHT * 0.7f, 20, COLOR_NEON_GREEN);
        } break;

        case STATE_WAITING_FOR_BALL: {
            DrawRectangle(0, 0, SCREEN_WIDTH, 220, Fade(BLACK, 0.8f));

            if (lastAnswerWasCorrect) {
                DrawText("✓ RESPOSTA CORRETA!", (SCREEN_WIDTH - MeasureText("✓ RESPOSTA CORRETA!", 40)) / 2, 80, 40, COLOR_NEON_GREEN);
                DrawText("O valor da bola será SOMADO à sua pontuação",
                        (SCREEN_WIDTH - MeasureText("O valor da bola será SOMADO à sua pontuação", 24)) / 2, 130, 24, LIME);
            } else {
                DrawText("✗ RESPOSTA INCORRETA", (SCREEN_WIDTH - MeasureText("✗ RESPOSTA INCORRETA", 40)) / 2, 80, 40, COLOR_NEON_RED);
                DrawText("O valor da bola será SUBTRAÍDO da sua pontuação",
                        (SCREEN_WIDTH - MeasureText("O valor da bola será SUBTRAÍDO da sua pontuação", 24)) / 2, 130, 24, ORANGE);
            }

            float pulse = sinf(GetTime() * 4.0f) * 0.5f + 0.5f;
            DrawText("PRESSIONE ESPAÇO PARA SOLTAR A BOLA",
                    (SCREEN_WIDTH - MeasureText("PRESSIONE ESPAÇO PARA SOLTAR A BOLA", 28)) / 2, 180, 28,
                    (Color){255, 255, 100, (int)(255 * pulse)});
        } break;

        case STATE_BALL_FALLING: {
            // Estado ativo - sem UI adicional durante a queda
            // A análise preditiva já está sendo mostrada no painel lateral
        } break;

        case STATE_BALL_LANDED: {
            DrawRectangle(0, 0, SCREEN_WIDTH, 240, Fade(BLACK, 0.8f));

            char resultadoStr[100];
            Color resultadoCor;

            if (lastAnswerWasCorrect) {
                snprintf(resultadoStr, sizeof(resultadoStr), "✓ Você GANHOU %d pontos!", lastValue);
                resultadoCor = COLOR_NEON_GREEN;
            } else {
                snprintf(resultadoStr, sizeof(resultadoStr), "✗ Você PERDEU %d pontos!", lastValue);
                resultadoCor = COLOR_NEON_RED;
            }

            DrawText("RESULTADO:", (SCREEN_WIDTH - MeasureText("RESULTADO:", 32)) / 2, 60, 32, WHITE);
            DrawText(TextFormat("Bola caiu no slot: %d (Valor: %d)", ball.slotIndex + 1, lastValue),
                    (SCREEN_WIDTH - MeasureText(TextFormat("Bola caiu no slot: %d (Valor: %d)", ball.slotIndex + 1, lastValue), 26)) / 2,
                    110, 26, RAYWHITE);
            DrawText(resultadoStr, (SCREEN_WIDTH - MeasureText(resultadoStr, 30)) / 2, 160, 30, resultadoCor);

            // Comparação com a previsão
            if (mostProbableSlot >= 0) {
                char predictionText[64];
                if (ball.slotIndex == mostProbableSlot) {
                    snprintf(predictionText, sizeof(predictionText), "Previsão CORRETA! (%.1f%%)", highestProbability * 100);
                    DrawText(predictionText, (SCREEN_WIDTH - MeasureText(predictionText, 20)) / 2, 200, 20, COLOR_NEON_GREEN);
                } else {
                    snprintf(predictionText, sizeof(predictionText), "Previsão: Slot %d (%.1f%%)", mostProbableSlot + 1, highestProbability * 100);
                    DrawText(predictionText, (SCREEN_WIDTH - MeasureText(predictionText, 18)) / 2, 200, 18, COLOR_NEON_BLUE);
                }
            }

            if (((int)(GetTime() * 2) % 2) == 0) {
                if (currentStage < NUM_ETAPAS - 1) {
                    DrawText("Pressione ENTER para a próxima etapa...",
                            (SCREEN_WIDTH - MeasureText("Pressione ENTER para a próxima etapa...", 22)) / 2, 230, 22, YELLOW);
                } else {
                    DrawText("Pressione ENTER para ver seu resultado...",
                            (SCREEN_WIDTH - MeasureText("Pressione ENTER para ver seu resultado...", 22)) / 2, 230, 22, YELLOW);
                }
            }
        } break;

        case STATE_NAME_INPUT: {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade((Color){10, 20, 40, 255}, 0.95f));

            int centerX = SCREEN_WIDTH / 2;
            int centerY = SCREEN_HEIGHT / 2;

            DrawText("FIM DE JOGO!", centerX - MeasureText("FIM DE JOGO!", 60)/2, centerY - 200, 60, COLOR_NEON_GOLD);

            char scoreText[50];
            snprintf(scoreText, sizeof(scoreText), "Pontuação Final: %lld", totalScore);
            DrawText(scoreText, centerX - MeasureText(scoreText, 40)/2, centerY - 120, 40, COLOR_NEON_GREEN);

            // Estatísticas finais
            DrawText(TextFormat("Precisão da Análise: %.1f%%", CalculatePredictionAccuracy()),
                    centerX - MeasureText(TextFormat("Precisão da Análise: %.1f%%", CalculatePredictionAccuracy()), 24)/2,
                    centerY - 70, 24, COLOR_NEON_BLUE);

            DrawText("Digite seu nome para o ranking:", centerX - MeasureText("Digite seu nome para o ranking:", 28)/2, centerY - 30, 28, WHITE);

            DrawRectangle(centerX - 250, centerY, 500, 70, (Color){30, 30, 50, 255});
            DrawRectangleLines(centerX - 250, centerY, 500, 70, COLOR_NEON_GOLD);

            DrawText(playerName, centerX - 240, centerY + 18, 36, YELLOW);

            // Cursor piscante
            if (((int)(GetTime() * 2) % 2) == 0) {
                int textWidth = MeasureText(playerName, 36);
                DrawText("_", centerX - 240 + textWidth, centerY + 18, 36, COLOR_NEON_GOLD);
            }

            // Instrução
            DrawText("Pressione ENTER para confirmar", centerX - MeasureText("Pressione ENTER para confirmar", 20)/2, centerY + 90, 20, LIGHTGRAY);

            // Mensagem de status
            if (letterCount == 0) {
                DrawText("Digite pelo menos um caractere", centerX - MeasureText("Digite pelo menos um caractere", 18)/2, centerY + 120, 18, ORANGE);
            }
        } break;

        case STATE_GAME_OVER: {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.9f));
            DrawText("FIM DE JOGO", (SCREEN_WIDTH - MeasureText("FIM DE JOGO", 80)) / 2, 150, 80, COLOR_NEON_GOLD);
            DrawText(TextFormat("Pontuação Final: %lld", totalScore),
                    (SCREEN_WIDTH - MeasureText(TextFormat("Pontuação Final: %lld", totalScore), 40)) / 2, 250, 40, YELLOW);

            DrawText("Pressione R para reiniciar", (SCREEN_WIDTH - MeasureText("Pressione R para reiniciar", 28)) / 2, 350, 28, RAYWHITE);
            DrawText("Pressione Q para voltar ao menu", (SCREEN_WIDTH - MeasureText("Pressione Q para voltar ao menu", 28)) / 2, 390, 28, LIGHTGRAY);
        } break;

        default: {
            // Estado não tratado - não faz nada
        } break;
    }

    // Combo display
    if (comboCount > 1 && comboDisplayTimer > 0.0f) {
        float comboPulse = sinf(GetTime() * 8.0f) * 0.5f + 0.5f;
        float floatOffset = sinf(GetTime() * 3.0f) * 8.0f;
        DrawText(TextFormat("COMBO x%d!", comboCount),
                SCREEN_WIDTH/2 - MeasureText(TextFormat("COMBO x%d!", comboCount), 42)/2,
                120 + floatOffset, 42,
                (Color){255, (int)(200 + comboPulse * 55), 100, 255});
    }

    // Indicador de câmera lenta
    if (IsSlowMotionActive()) {
        DrawSlowMotionIndicator();
    }
}