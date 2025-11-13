#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOMINMAX

#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define NUM_PINS_X 11
#define NUM_PINS_Y 9
#define PIN_SPACING 60
#define PIN_RADIUS 5
#define BALL_RADIUS 11
#define GRAVITY 500.0f
#define SLOT_COUNT (NUM_PINS_X + 1)

typedef struct {
    float x, y;
    float vx, vy;
    int   active;
    int   slotIndex;
} Ball;

typedef struct {
    float x, y;
} Pin;

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 700;
    InitWindow(screenWidth, screenHeight, "The Wall / Plinko physics sem Vector2");

    srand((unsigned)time(NULL));
    SetTargetFPS(60);

    // ----- Cria pinos -----
    Pin pins[NUM_PINS_X * NUM_PINS_Y];
    int pinCount = 0;
    for (int y = 0; y < NUM_PINS_Y; y++) {
        for (int x = 0; x < NUM_PINS_X; x++) {
            float offset = (y % 2 == 0) ? 0 : PIN_SPACING / 2.0f;
            pins[pinCount].x = screenWidth / 2.0f - NUM_PINS_X * PIN_SPACING / 2 + offset + x * PIN_SPACING;
            pins[pinCount].y = 100 + y * PIN_SPACING;
            pinCount++;
        }
    }

    // Slots na base
    float slotWidth = PIN_SPACING;
    float baseY = screenHeight - 60;
    int slotValues[SLOT_COUNT];
    for (int i = 0; i < SLOT_COUNT; i++) {
        slotValues[i] = (rand() % 2 == 0) ? 1000 : -500;
    }

    Ball ball = {0};
    ball.active = 0;

    // ----- Loop principal -----
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Soltar bola nova
        if (IsKeyPressed(KEY_SPACE) && !ball.active) {
            ball.x = screenWidth / 2.0f;
            ball.y = 40;
            ball.vx = 0;
            ball.vy = 0;
            ball.active = 1;
            ball.slotIndex = -1;
        }

        // ----- Atualiza física -----
        if (ball.active) {
            // gravidade
            ball.vy += GRAVITY * dt;

            // atualiza posição
            ball.x += ball.vx * dt;
            ball.y += ball.vy * dt;

            // colisão com pinos
            for (int i = 0; i < pinCount; i++) {
                float dx = ball.x - pins[i].x;
                float dy = ball.y - pins[i].y;
                float dist = sqrtf(dx*dx + dy*dy);
                float minDist = BALL_RADIUS + PIN_RADIUS;

                if (dist < minDist && dist > 0.0001f) {
                    // normal de colisão
                    float nx = dx / dist;
                    float ny = dy / dist;

                    // empurra para fora do pino
                    float pen = minDist - dist;
                    ball.x += nx * pen;
                    ball.y += ny * pen;

                    // componente da velocidade na direção da normal
                    float vDotN = ball.vx * nx + ball.vy * ny;

                    // reflexão simples
                    ball.vx = ball.vx - 2.0f * vDotN * nx;
                    ball.vy = ball.vy - 2.0f * vDotN * ny;

                    // amortecimento da energia para não quicar demais
                    ball.vx *= 0.6f;
                    ball.vy *= 0.6f;

                    // pequeno impulso lateral randômico
                    ball.vx += ((rand() % 200) - 100) / 100.0f * 40.0f;

                    break;
                }
            }

            // parede esquerda/direita
            if (ball.x < BALL_RADIUS) {
                ball.x = BALL_RADIUS;
                ball.vx *= -0.6f;
            }
            if (ball.x > screenWidth - BALL_RADIUS) {
                ball.x = screenWidth - BALL_RADIUS;
                ball.vx *= -0.6f;
            }

            // base
            if (ball.y > baseY - BALL_RADIUS) {
                ball.y = baseY - BALL_RADIUS;
                int idx = (int)((ball.x - (screenWidth / 2.0f - NUM_PINS_X * PIN_SPACING / 2)) / slotWidth);
                if (idx < 0) idx = 0;
                if (idx >= SLOT_COUNT) idx = SLOT_COUNT - 1;
                ball.slotIndex = idx;
                ball.active = 0;
            }
        }

        // ----- Desenho -----
        BeginDrawing();
        ClearBackground((Color){5,15,40,255});

        // pinos
        for (int i = 0; i < pinCount; i++)
            DrawCircle((int)pins[i].x, (int)pins[i].y, PIN_RADIUS, RAYWHITE);

        // base + slots
        DrawRectangle(0, baseY, screenWidth, 60, DARKGRAY);
        for (int i = 0; i < SLOT_COUNT; i++) {
            float x = screenWidth / 2.0f - NUM_PINS_X * PIN_SPACING / 2 + i * slotWidth;
            DrawLine(x, baseY, x, screenHeight, RAYWHITE);
            char txt[16];
            sprintf(txt, "%+d", slotValues[i]);
            DrawText(txt, x + slotWidth/2 - 20, baseY + 25, 16,
                     (slotValues[i] > 0) ? GREEN : RED);
        }

        // bola
        if (ball.active) {
            DrawCircle((int)ball.x, (int)ball.y, BALL_RADIUS, GOLD);
        } else {
            DrawText("Pressione [ESPACO] para soltar bola", 220, 20, 20, YELLOW);
            if (ball.slotIndex >= 0) {
                int val = slotValues[ball.slotIndex];
                DrawText(TextFormat("Caiu no slot %d  Valor: %d", ball.slotIndex, val),
                         250, 60, 20, (val>0)?GREEN:RED);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}