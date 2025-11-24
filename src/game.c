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
#include "core/hud.h"

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

    switch (currentState) {
        case STATE_START_SCREEN: {
            DrawStartScreenHUD();
        } break;

        case STATE_ASKING_QUESTION: {
            DrawQuiz(currentStage);
        } break;

        case STATE_WAITING_FOR_BALL: {
            DrawWaitingForBallHUD(lastAnswerWasCorrect);
        } break;

        case STATE_BALL_FALLING: {
            // Estado ativo - sem UI adicional durante a queda
        } break;

        case STATE_BALL_LANDED: {
            DrawBallLandedHUD(lastAnswerWasCorrect, currentStage);
        } break;

        case STATE_NAME_INPUT: {
            DrawNameInputHUD(playerName, letterCount, totalScore);
        } break;

        case STATE_GAME_OVER: {
            DrawGameOverHUD(totalScore);
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