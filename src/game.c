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
#include "core/ball.h"
#include "core/quiz.h"

typedef enum {
    STATE_START_SCREEN,
    STATE_ASKING_QUESTION,
    STATE_WAITING_FOR_BALL,
    STATE_BALL_FALLING,
    STATE_BALL_LANDED,
    STATE_NAME_INPUT,
    STATE_GAME_OVER
} InternalGameState;

static Pin pins[NUM_PINS_X * NUM_PINS_Y];
static int pinCount = 0;
static float firstPinY = 120.0f;
static float firstSlotX;
static float slotWidth;
static float baseY;
static char playerName[MAX_NAME_LENGTH + 1] = { 0 };
static int letterCount = 0;
static const int gameAreaHeight = 800;
static InternalGameState currentState;
static int lastAnswerWasCorrect;
static Color slotColor;
static int comboCount = 0;
static float comboDisplayTimer = 0.0f;

int currentStage;
int slotValues[SLOT_COUNT];
int slotCounts[SLOT_COUNT];
int totalBolas;
long long totalScore;
extern GameScreen currentScreen;

void InitGame(void) {
    currentState = STATE_START_SCREEN;
    currentStage = 0;
    totalScore = 0;
    totalBolas = 0;
    lastAnswerWasCorrect = 0;
    slotColor = COLOR_NEON_BLUE;
    letterCount = 0;
    comboCount = 0;
    comboDisplayTimer = 0.0f;
    memset(playerName, 0, sizeof(playerName));

    InitPredictionSystem();
    InitBalls();
    InitParticles();

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
}

void UpdateGame(void) {
    float dt = GetFrameTime();
    UpdateSlowMotion();
    dt = ApplyTimeScale(dt);

    if (comboDisplayTimer > 0.0f) {
        comboDisplayTimer -= dt;
    }

    UpdateScreenShake(dt);
    UpdateParticles(dt);

    switch (currentState) {
        case STATE_START_SCREEN: {
            if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP)) {
                currentState = STATE_ASKING_QUESTION;
            }
        } break;

        case STATE_ASKING_QUESTION: {
            int result = UpdateQuiz();
            if (result != -1) {
                if (result == 1) {
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

            if ((IsKeyPressed(KEY_SPACE) || IsGestureDetected(GESTURE_TAP))) {
                SpawnBall(lastAnswerWasCorrect);
                currentState = STATE_BALL_FALLING;
            }
        } break;

        case STATE_BALL_FALLING: {
            if (AnyBallActive()) {
                UpdateBalls(pins, pinCount, baseY, firstSlotX, slotWidth, lastAnswerWasCorrect, dt);

                Ball* refBall = GetFirstActiveBall();
                if (refBall) {
                    CalculateRealTimeProbabilities(refBall->x, refBall->y);
                    UpdatePredictionPaths(refBall->x, refBall->y);
                }
            } else {
                currentState = STATE_BALL_LANDED;
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
    DrawRectangleGradientV(0, baseY, SCREEN_WIDTH, gameAreaHeight - baseY, (Color){40, 45, 70, 255}, (Color){25, 30, 50, 255});

    for (int i = 0; i < SLOT_COUNT; i++) {
        float x = firstSlotX + i * slotWidth;
        DrawLine(x, baseY, x, gameAreaHeight, (Color){80, 90, 140, 255});

        char txt[16];
        sprintf(txt, "%d", slotValues[i]);
        int textWidth = MeasureText(txt, 20);
        DrawText(txt, x + slotWidth/2 - textWidth/2, baseY + 20, 20, slotColor);
    }

    if (AnyBallActive()) {
        DrawPredictionPaths();
        DrawPredictionCurve();
    }

    DrawParticles();
    DrawBalls();

    EndMode2D();

    // Painéis e gráficos da análise preditiva
    DrawPredictionPanel(totalBolas, currentStage, comboCount, slotCounts, GetLastBallSlot(), AnyBallActive(), GetLastBallSlot());
    DrawPredictionChart(totalBolas, slotCounts, GetLastBallSlot(), baseY);

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
            DrawQuiz(currentStage);
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
                snprintf(resultadoStr, sizeof(resultadoStr), "✓ Rodada concluída! Confira sua pontuação!");
                resultadoCor = COLOR_NEON_GREEN;
            } else {
                snprintf(resultadoStr, sizeof(resultadoStr), "✗ Rodada concluída! Pontos reduzidos!");
                resultadoCor = COLOR_NEON_RED;
            }
            DrawText(resultadoStr, (SCREEN_WIDTH - MeasureText(resultadoStr, 30)) / 2, 130, 30, resultadoCor);

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