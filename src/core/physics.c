#include "physics.h"
#include "effects.h"
#include "game_common.h"
#include <math.h>
#include <stdlib.h>

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
            ball->x += nx * penetration * 0.5f;
            ball->y += ny * penetration * 0.5f;

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
    if (ball->x < BALL_RADIUS) {
        ball->x = BALL_RADIUS;
        ball->vx = fabsf(ball->vx) * FRICTION;
        CreateParticles(ball->x, ball->y, COLOR_NEON_BLUE, 5);
    } else if (ball->x > SCREEN_WIDTH - BALL_RADIUS) {
        ball->x = SCREEN_WIDTH - BALL_RADIUS;
        ball->vx = -fabsf(ball->vx) * FRICTION;
        CreateParticles(ball->x, ball->y, COLOR_NEON_BLUE, 5);
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