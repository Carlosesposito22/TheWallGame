#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ranking.h"
#include "commons.h"
#include "core/effects.h"
#include "core/game_common.h"
#include "core/physics.h"
#include "core/particles.h"
#include "core/prediction.h"

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
static InternalGameState currentState;
static int slotCounts[SLOT_COUNT];
static int totalBolas;
static Ball ball;
static int currentStage;
static long long totalScore;
static int lastAnswerWasCorrect;
static int lastValue;
static Color slotColor;
static float uiPulse = 0.0f;
static int comboCount = 0;
static float comboDisplayTimer = 0.0f;

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

extern GameScreen currentScreen;

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

    InitPredictionSystem();

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
            ResetPredictionText();

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

                if (UpdateBallPhysics(&ball,pins, pinCount,baseY, firstSlotX, slotWidth,slotCounts, &totalBolas,slotValues, &totalScore,lastAnswerWasCorrect,dt)) {
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

    DrawParticles();

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
    DrawRectangle(statsPanelX + 20, 125, (int)(uncertaintyBarWidth * GetUncertaintyLevel()), 12,
                 (Color){255, (int)(255 * GetUncertaintyLevel()), (int)(100 * (1.0f - GetUncertaintyLevel())), 255});
    DrawText(TextFormat("Incerteza: %.0f%%", GetUncertaintyLevel() * 100), statsPanelX + 20, 140, 14, LIGHTGRAY);

    // Análise preditiva em tempo real
    DrawText("ANÁLISE PREDITIVA:", statsPanelX + 20, 165, 16, COLOR_NEON_BLUE);

    if (ball.active && ball.slotIndex == -1) {
        // Textos dinâmicos de análise
        DrawText(GetAnalysisLine(0), statsPanelX + 25, 190, 16, WHITE);
        DrawText(GetAnalysisLine(1), statsPanelX + 25, 215, 14, LIGHTGRAY);
        DrawText(GetAnalysisLine(2), statsPanelX + 25, 235, 14, LIGHTGRAY);

        // Slot mais provável
        if (mostProbableSlot >= 0) {
            DrawText(TextFormat("Mais provável: Slot %d (%.1f%%)",
                               mostProbableSlot + 1, highestProbability * 100),
                    statsPanelX + 25, 260, 14, COLOR_NEON_GREEN);
        }

        // Pinos restantes
        DrawText(TextFormat("Pinos restantes: %d/%d", GetPinsRemaining(), NUM_PINS_Y),
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