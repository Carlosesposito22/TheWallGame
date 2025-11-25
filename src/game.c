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

#define WALL_HEIGHT_RATIO 1.5f
#define WALL_ROWS 14
#define TOP_SLOTS 7
#define BOTTOM_SLOTS 13
#define WALL_BASE_Y 750.0f
#define WALL_TOP_Y 100.0f
#define WALL_HEIGHT_RATIO 1.5f

#define MAX_PINS 500 

static float leftWallX;
static float rightWallX;
static Music gameMusic;

typedef enum {
    STATE_START_SCREEN,
    STATE_CHOOSING_SLOT,
    STATE_ASKING_QUESTION,
    STATE_WAITING_FOR_BALL,
    STATE_BALL_FALLING,
    STATE_BALL_LANDED,
    STATE_NAME_INPUT,
    STATE_GAME_OVER
} InternalGameState;

static int selectedSlot = 0;
static Pin pins[MAX_PINS];
static int pinCount = 0;
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

    InitQuiz();
    InitPredictionSystem();
    InitBalls();
    InitParticles();

    gameMusic = LoadMusicStream("assets/musicagamex.mp3"); 
    PlayMusicStream(gameMusic); // Inicia 
    SetMusicVolume(gameMusic, 0.9f);

    // Define uma margem fixa para garantir que o jogo caiba na tela
    const float wallMargin = 350.0f; 
    
    leftWallX  = wallMargin;
    rightWallX = SCREEN_WIDTH - wallMargin;

    // Área total utilizável entre as paredes
    float usableWidth = rightWallX - leftWallX;

    pinCount = 0;
    float wallHeight = WALL_BASE_Y - WALL_TOP_Y;

    // A largura efetiva para os centros dos pinos é menor, 
    // pois o centro do pino mais externo deve estar a PIN_RADIUS de distância da parede.
    float effectiveWidth = usableWidth - (2.0f * PIN_RADIUS); 

    // Calcula quantos pinos cabem no espaço efetivo (com espaçamento de PIN_SPACING)
    // Se PIN_SPACING for 2 * PIN_RADIUS, o cálculo é exato.
    // Adicionamos +1.0f na divisão para garantir que o último pino seja contado
    // se o espaço for exato para ele, e então truncamos.
    int maxPinsPerRowCount = (int)(effectiveWidth / PIN_SPACING) + 1;
    
    // A variável rowRealWidth não é mais necessária, 
    // usaremos o cálculo direto para centralizar.

    for (int row = 0; row < WALL_ROWS; row++) {
        float t = (float)row / (WALL_ROWS - 1);

        // O número de pinos varia se a linha for par ou ímpar
        int pinsPerRow = maxPinsPerRowCount;

        // Linhas intercaladas (o primeiro pino é deslocado)
        if (row % 2 != 0) {
            pinsPerRow = maxPinsPerRowCount - 1; // Linha ímpar tem um pino a menos para o padrão Galton
        }
        
        // 1. O ponto de partida para o *centro* do primeiro pino:
        // Deve ser o LeftWallX + PIN_RADIUS
        float idealStart = leftWallX + PIN_RADIUS;

        // 2. A largura que os pinos dessa linha realmente ocuparão:
        float rowRealOccupiedWidth = (pinsPerRow - 1) * PIN_SPACING;

        // 3. Centralização: 
        // O ponto de partida (startX) é ajustado para centralizar o conjunto de pinos dentro da largura efetiva.
        float startX = idealStart + (effectiveWidth - rowRealOccupiedWidth) * 0.5f;

        // 4. Ajuste para linhas ímpares:
        if (row % 2 != 0) {
            // Se a linha ímpar tiver um pino a menos e for centralizada, o startX já está correto.
            // Se você quiser que as linhas ímpares fiquem alinhadas horizontalmente com as pares,
            // (ou seja, deslocadas por 0.5 * PIN_SPACING), o cálculo já contempla isso.
            // Não é necessário adicionar um ajuste de 0.5f * PIN_SPACING se estivermos centralizando.
        }

        for (int col = 0; col < pinsPerRow; col++) {
            if (pinCount >= MAX_PINS) break;

            float x = startX + col * PIN_SPACING;
            float y = WALL_TOP_Y + t * wallHeight;

            // NENHUMA VERIFICAÇÃO DE MARGEM DEVE SER NECESSÁRIA AQUI.
            // Se o cálculo estiver correto, o primeiro pino deve estar em 
            // leftWallX + PIN_RADIUS e o último em rightWallX - PIN_RADIUS.
            
            pins[pinCount].x = x;
            pins[pinCount].y = y;
            pins[pinCount].color = COLOR_NEON_BLUE;
            pins[pinCount].visible = 1;
            pinCount++;
        }
    }

    // Gavetas de baixo (Bottom Slots)
    slotWidth  = usableWidth / (float)BOTTOM_SLOTS;
    firstSlotX = leftWallX;
    baseY      = WALL_BASE_Y;

    // Paredes Físicas (Mantendo a correção da física: Esquerda Base->Topo)
    AddWall(leftWallX - 1.0f, WALL_BASE_Y, leftWallX - 1.0f, WALL_TOP_Y);
    AddWall(rightWallX + 1.0f, WALL_TOP_Y, rightWallX + 1.0f, WALL_BASE_Y);

    // Distribui valores
    int valoresBase[BOTTOM_SLOTS] = {
        1, 10000, 100, 15000, 10, 30000, 100,
        45000, 100, 60000, 100, 150000, 10
    };
    int limit = (BOTTOM_SLOTS < 15) ? BOTTOM_SLOTS : 15;
    for (int i = 0; i < limit; i++) {
        slotValues[i] = valoresBase[i];
    }
}

void UpdateGame(void) {
    float dt = GetFrameTime();
    UpdateSlowMotion();
    dt = ApplyTimeScale(dt);

    UpdateMusicStream(gameMusic);

    if (comboDisplayTimer > 0.0f) {
        comboDisplayTimer -= dt;
    }

    UpdateScreenShake(dt);
    UpdateParticles(dt);

    switch (currentState) {
        case STATE_START_SCREEN: {
            if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP)) {
                currentState = STATE_CHOOSING_SLOT;
            }
        } break;

        case STATE_CHOOSING_SLOT: {
            // Lógica para mover a seleção do slot com teclas de seta (esquerda/direita)
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                selectedSlot--;
                if (selectedSlot < 0) selectedSlot = 0;
            }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                selectedSlot++;
                // TOP_SLOTS é definido como 7
                if (selectedSlot >= TOP_SLOTS) selectedSlot = TOP_SLOTS - 1; 
            }

            // Confirmação: passa para a próxima fase (pergunta)
            if (IsKeyPressed(KEY_ENTER)) {
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
                SpawnBall(lastAnswerWasCorrect, selectedSlot);
                currentState = STATE_BALL_FALLING;
            }
        } break;

        case STATE_BALL_FALLING: {
            if (AnyBallActive()) {
                UpdateBalls(pins, pinCount, baseY, firstSlotX, slotWidth, lastAnswerWasCorrect, dt);

                Ball* refBall = GetFirstActiveBall(); // Mantendo a chamada para simplificar
                
                if (refBall) {
                    CalculateRealTimeProbabilities(refBall->x, refBall->y);
                    UpdatePredictionPaths(refBall->x, refBall->y);
                } else { 
                    currentState = STATE_BALL_LANDED;
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
                    currentState = STATE_CHOOSING_SLOT;
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
        Color gradColor = {
            (unsigned char)(10 + 40 * t),
            (unsigned char)(20 + 40 * t),
            (unsigned char)(40 + 80 * t),
            255
        };
        DrawRectangle(0, i, SCREEN_WIDTH, 1, gradColor);
    }

    // Moldura arqueada com LED
    int edgeThickness = 10;
    for (int i = 0; i < edgeThickness; i++) {
        DrawLineEx((Vector2){i, WALL_TOP_Y + i},
                (Vector2){SCREEN_WIDTH - i, WALL_TOP_Y + i},
                2, COLOR_NEON_BLUE);
        DrawLineEx((Vector2){i, WALL_BASE_Y - i},
                (Vector2){SCREEN_WIDTH - i, WALL_BASE_Y - i},
                2, COLOR_NEON_BLUE);
    }

    // Slots Superiores
    float totalWallWidth = rightWallX - leftWallX;
    float topSlotWidth = totalWallWidth / (float)TOP_SLOTS;
    
    for (int i = 0; i < TOP_SLOTS; i++) {
        float x0 = leftWallX + i * topSlotWidth;
        float xMid = x0 + topSlotWidth / 2;

        if (currentState == STATE_CHOOSING_SLOT && i == selectedSlot) {
            DrawRectangle(x0 + 1, WALL_TOP_Y - 41, topSlotWidth - 2, 32, (Color){255, 200, 0, 80}); 
        }
        
        DrawRectangleLines(x0 + 2, WALL_TOP_Y - 40, topSlotWidth - 4, 30, (Color){100, 150, 255, 200});
        DrawText(TextFormat("%d", i + 1), (int)(xMid - 6), WALL_TOP_Y - 38, 24, COLOR_NEON_BLUE);
    }

    // Paredes laterais visuais
    DrawRectangle(leftWallX - 8, WALL_TOP_Y, 16, WALL_BASE_Y - WALL_TOP_Y, (Color){20, 60, 160, 200});
    DrawRectangle(rightWallX - 8, WALL_TOP_Y, 16, WALL_BASE_Y - WALL_TOP_Y, (Color){20, 60, 160, 200});
    DrawRectangleLines(leftWallX - 8, WALL_TOP_Y, 16, WALL_BASE_Y - WALL_TOP_Y, COLOR_NEON_BLUE);
    DrawRectangleLines(rightWallX - 8, WALL_TOP_Y, 16, WALL_BASE_Y - WALL_TOP_Y, COLOR_NEON_BLUE);

    // Desenha pinos
    for (int i = 0; i < pinCount; i++) {
        if (pins[i].visible) {
            DrawCircle((int)pins[i].x, (int)pins[i].y, PIN_RADIUS * 1.6f, (Color){0, 60, 255, 80}); 
            DrawCircle((int)pins[i].x, (int)pins[i].y, PIN_RADIUS, pins[i].color);
            DrawCircleLines((int)pins[i].x, (int)pins[i].y, PIN_RADIUS, (Color){180, 200, 255, 255});
        }
    }

    // Desenha gavetas de baixo
    for (int i = 0; i < BOTTOM_SLOTS; i++) {
        float x = firstSlotX + i * slotWidth;
        DrawRectangleLines(x, baseY, slotWidth, 40, (Color){0, 150, 255, 255});
        char txt[16];
        sprintf(txt, "R$%d", slotValues[i]);
        
        int fontSize = 18;
        if (slotWidth < 50) fontSize = 14; 
        
        int textWidth = MeasureText(txt, fontSize);
        DrawText(txt, x + slotWidth/2 - textWidth/2, baseY + 10, fontSize, (Color){0, 200, 255, 255});
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

    DrawPredictionPanel(totalBolas, currentStage, comboCount, slotCounts, GetLastBallSlot(), AnyBallActive(), GetLastBallSlot());
    DrawPredictionChart(totalBolas, slotCounts, GetLastBallSlot(), baseY);

    DrawRectangle(20, 20, 280, 100, COLOR_UI_BG);
    DrawRectangleLines(20, 20, 280, 100, COLOR_UI_BORDER);
    DrawText("PONTUAÇÃO", 40, 30, 22, COLOR_NEON_GOLD);
    DrawText(TextFormat("%lld", totalScore), 40, 60, 32, WHITE);

    switch (currentState) {
        case STATE_START_SCREEN: DrawStartScreenHUD(); break;
        case STATE_CHOOSING_SLOT: DrawChoosingSlotHUD(); break;
        case STATE_ASKING_QUESTION: DrawQuiz(currentStage); break;
        case STATE_WAITING_FOR_BALL: DrawWaitingForBallHUD(lastAnswerWasCorrect); break;
        case STATE_BALL_FALLING: break;
        case STATE_BALL_LANDED: DrawBallLandedHUD(lastAnswerWasCorrect, currentStage); break;
        case STATE_NAME_INPUT: DrawNameInputHUD(playerName, letterCount, totalScore); break;
        case STATE_GAME_OVER: DrawGameOverHUD(totalScore); break;
    }

    if (comboCount > 1 && comboDisplayTimer > 0.0f) {
        float comboPulse = sinf(GetTime() * 8.0f) * 0.5f + 0.5f;
        float floatOffset = sinf(GetTime() * 3.0f) * 8.0f;
        DrawText(TextFormat("COMBO x%d!", comboCount),
                SCREEN_WIDTH/2 - MeasureText(TextFormat("COMBO x%d!", comboCount), 42)/2,
                120 + floatOffset, 42,
                (Color){255, (int)(200 + comboPulse * 55), 100, 255});
    }

    if (IsSlowMotionActive()) {
        DrawSlowMotionIndicator();
    }
}