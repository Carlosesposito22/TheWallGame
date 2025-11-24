#include "ball.h"
#include "physics.h"
#include "particles.h"
#include "effects.h"
#include "game_common.h"
#include <math.h>
#include <stdlib.h>

static Ball balls[MAX_BALLS];
static int ballCount = 0;
static float spawnPositions[TOP_SLOTS];
static int lastBallSlot = -1;
extern int slotCounts[SLOT_COUNT];
extern int totalBolas;
extern int slotValues[SLOT_COUNT];
extern long long totalScore;

void InitBalls(void) {
    ballCount = 0;
    for (int i = 0; i < MAX_BALLS; i++) {
        balls[i].active = 0;
        balls[i].scale = 1.0f;
        balls[i].rotation = 0.0f;
        balls[i].rotationSpeed = 0.0f;
        balls[i].slotIndex = -1;
        balls[i].color = COLOR_NEON_GOLD;
    }

    // Calcula as posições horizontais dos 7 possíveis slots superiores
    float gap = SCREEN_WIDTH / (float)(TOP_SLOTS + 1);
    for (int i = 0; i < TOP_SLOTS; i++) {
        spawnPositions[i] = gap * (i + 1);
    }
}

bool SpawnBall(bool lastAnswerWasCorrect) {
    if (ballCount >= MAX_BALLS) return false;

    // Encontra um slot de spawn aleatório entre 7 disponíveis
    int slot = rand() % TOP_SLOTS;
    float x = spawnPositions[slot];
    float y = 60.0f;        // Altura inicial fixa

    // Tenta achar uma posição levemente deslocada se houver colisão
    for (int t = 0; t < 5; t++) {
        bool overlap = false;
        for (int i = 0; i < ballCount; i++) {
            float dx = x - balls[i].x;
            float dy = y - balls[i].y;
            if (sqrtf(dx*dx + dy*dy) < BALL_RADIUS * 2) {
                overlap = true;
                x += RandomFloat(-20, 20);
                break;
            }
        }
        if (!overlap) break;
    }

    Ball *b = &balls[ballCount++];
    b->x = x;
    b->y = y;
    b->vx = RandomFloat(-100, 100);
    b->vy = 0;
    b->active = 1;
    b->slotIndex = -1;
    b->color = lastAnswerWasCorrect ? COLOR_NEON_GREEN : COLOR_NEON_RED;
    b->scale = 1.3f;
    b->rotationSpeed = b->vx * 0.02f;
    StartScreenShake(2.0f);

    return true;
}

void UpdateBalls(Pin *pins, int pinCount, float baseY, float firstSlotX, float slotWidth, bool lastAnswerWasCorrect, float dt) {
    for (int i = 0; i < ballCount; i++) {
        Ball *b = &balls[i];
        if (!b->active) continue;

        if (UpdateBallPhysics(b, pins, pinCount, baseY, firstSlotX, slotWidth,
                              slotCounts, &totalBolas, slotValues, &totalScore,
                              lastAnswerWasCorrect, dt)) {
            b->active = 0;
            lastBallSlot = b->slotIndex;
        }

        UpdateBallTrail(b->x, b->y, dt);

        // Colisão bola–bola
        for (int j = i + 1; j < ballCount; j++) {
            Ball *b2 = &balls[j];
            if (!b2->active) continue;

            float dx = b2->x - b->x;
            float dy = b2->y - b->y;
            float dist = sqrtf(dx * dx + dy * dy);
            float minDist = 2 * BALL_RADIUS;

            if (dist > 0 && dist < minDist) {
                float overlap = 0.5f * (minDist - dist);
                float nx = dx / dist;
                float ny = dy / dist;
                // separar
                b->x -= nx * overlap;
                b->y -= ny * overlap;
                b2->x += nx * overlap;
                b2->y += ny * overlap;

                // troca de velocidade
                float dvx = b2->vx - b->vx;
                float dvy = b2->vy - b->vy;
                float dot = dvx * nx + dvy * ny;
                if (dot < 0) {
                    float impulse = dot * (1 + ELASTICITY);
                    b->vx += impulse * nx;
                    b->vy += impulse * ny;
                    b2->vx -= impulse * nx;
                    b2->vy -= impulse * ny;
                }
                CreateParticles((b->x + b2->x) / 2, (b->y + b2->y) / 2, COLOR_NEON_BLUE, 4);
            }
        }

        b->rotation += b->rotationSpeed * dt;
        b->scale = MathLerp(b->scale, 1.0f, 5.0f * dt);
    }

    // limpa bolas finalizadas
    int newCount = 0;
    for (int i = 0; i < ballCount; i++)
        if (balls[i].active || balls[i].slotIndex >= 0)
            balls[newCount++] = balls[i];
    ballCount = newCount;
}

void DrawBalls(void) {
    for (int i = 0; i < ballCount; i++) {
        Ball *b = &balls[i];
        if (!b->active && b->slotIndex < 0) continue;

        DrawCircle((int)b->x, (int)b->y, BALL_RADIUS * b->scale + 3,
                   (Color){255, 255, 255, 80});
        DrawCircle((int)b->x, (int)b->y, BALL_RADIUS * b->scale, b->color);
        DrawCircleLines((int)b->x, (int)b->y, BALL_RADIUS * b->scale, WHITE);

        float markerX = b->x + cosf(b->rotation) * BALL_RADIUS * 0.6f;
        float markerY = b->y + sinf(b->rotation) * BALL_RADIUS * 0.6f;
        DrawCircle((int)markerX, (int)markerY, BALL_RADIUS * 0.2f, (Color){255, 255, 255, 180});
    }
}

bool AnyBallActive(void) {
    for (int i = 0; i < ballCount; i++)
        if (balls[i].active)
            return true;
    return false;
}

int GetLastBallSlot(void) {
    return lastBallSlot;
}

Ball* GetFirstActiveBall(void) {
    for (int i = 0; i < ballCount; i++) {
        if (balls[i].active)
            return &balls[i];
    }
    return NULL;
}