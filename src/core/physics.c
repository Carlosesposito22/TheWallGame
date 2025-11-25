#include "physics.h"
#include "effects.h"
#include "game_common.h"
#include <math.h>
#include <stdlib.h>

typedef struct {
    float x1, y1;
    float x2, y2;
} Wall;

#define MAX_WALLS 8
static Wall walls[MAX_WALLS];
static int wallCount = 0;

int UpdateBallPhysics(
    Ball* ball,
    Pin* pins, int pinCount,
    float baseY, float firstSlotX, float slotWidth,
    int slotCounts[], int* totalBolas,
    int slotValues[], long long* totalScore,
    int lastAnswerWasCorrect,
    float dt
) {
    if (!ball->active) return 0;

    ball->vy += GRAVITY * dt;
    ball->vx *= AIR_RESISTANCE;
    ball->vy *= AIR_RESISTANCE;

    ball->x += ball->vx * dt;
    ball->y += ball->vy * dt;

    ball->rotationSpeed = MathLerp(ball->rotationSpeed, ball->vx * 0.015f, 5.0f * dt);
    ball->rotation += ball->rotationSpeed;
    ball->scale = MathLerp(ball->scale, 1.0f, 8.0f * dt);

    // ------------------------------
    // Colisão com pinos
    // ------------------------------
    for (int i = 0; i < pinCount; i++) {
        if (!pins[i].visible) continue;

        float dx = ball->x - pins[i].x;
        float dy = ball->y - pins[i].y;
        float dist = sqrtf(dx*dx + dy*dy);
        float minDist = BALL_RADIUS + PIN_RADIUS;

        if (dist < minDist && dist > 0.0001f) {
            float nx = dx / dist;
            float ny = dy / dist;
            float penetration = minDist - dist;
            ball->x += nx * penetration * 1.01f;
            ball->y += ny * penetration * 1.01f;

            float dotProduct = ball->vx * nx + ball->vy * ny;
            ball->vx = (ball->vx - 2.0f * dotProduct * nx) * ELASTICITY;
            ball->vy = (ball->vy - 2.0f * dotProduct * ny) * ELASTICITY;

            pins[i].color = YELLOW;
            CreateParticles(pins[i].x, pins[i].y, YELLOW, 8);
            ball->rotationSpeed += ball->vx * 0.03f;
        }
    }

    // ------------------------------
    // Colisão com paredes laterais
    // ------------------------------
    for (int i = 0; i < wallCount; i++) {
        Wall *w = &walls[i];

        // Vetor da parede
        float dx = w->x2 - w->x1;
        float dy = w->y2 - w->y1;
        float length = sqrtf(dx * dx + dy * dy);
        if (length < 0.0001f) continue;

        // Normal unitária da parede
        float nx = -dy / length;
        float ny = dx / length;

        // Distância da bola à parede (positivo fora, negativo dentro)
        float dist = (ball->x - w->x1) * nx + (ball->y - w->y1) * ny;

        if (dist < BALL_RADIUS) {
            // Corrige penetração
            float penetration = BALL_RADIUS - dist;
            ball->x += nx * penetration;
            ball->y += ny * penetration;

            // Rebate velocidade com elasticidade e atrito
            float dot = ball->vx * nx + ball->vy * ny;
            if (dot < 0.0f) {
                ball->vx -= (1.0f + ELASTICITY) * dot * nx;
                ball->vy -= (1.0f + ELASTICITY) * dot * ny;
            }

            CreateParticles(ball->x, ball->y, COLOR_NEON_BLUE, 5);
        }
    }

    // ------------------------------
    // Aterrissagem nos slots inferiores
    // ------------------------------
    if (ball->y > baseY - BALL_RADIUS) {
        ball->y = baseY - BALL_RADIUS;

        int idx = (int)((ball->x - firstSlotX) / slotWidth);
        if (idx < 0) idx = 0;
        else if (idx >= SLOT_COUNT) idx = SLOT_COUNT - 1;

        ball->slotIndex = idx;
        ball->active = 0;
        slotCounts[idx]++;
        (*totalBolas)++;

        int lastValue = slotValues[idx];
        int delta = lastAnswerWasCorrect ? lastValue : -lastValue;
        *totalScore += delta;

        Color c = lastAnswerWasCorrect ? COLOR_NEON_GREEN : COLOR_NEON_RED;
        CreateParticles(ball->x, ball->y, c, 15);
        StartScreenShake(lastAnswerWasCorrect ? 5.0f : 10.0f);

        return 1; // Bola aterrissou
    }

    return 0; // Continua caindo
}

void AddWall(float x1, float y1, float x2, float y2) {
    if (wallCount >= MAX_WALLS) return;    // Limite de segurança
    walls[wallCount].x1 = x1;
    walls[wallCount].y1 = y1;
    walls[wallCount].x2 = x2;
    walls[wallCount].y2 = y2;
    wallCount++;
}